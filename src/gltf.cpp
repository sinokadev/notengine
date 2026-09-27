#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#include <stb/stb_image.h>

#include <knot/scene.h>
#include <knot/utility/mesh_helper.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace knot {
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
glm::vec3 unit(glm::vec3 value, glm::vec3 fallback = glm::vec3(0, 1, 0)) {
    float length = glm::dot(value, value);
    return std::isfinite(length) && length > 1e-20f ? value / std::sqrt(length) : fallback;
}

// Imported textures belong to the material, including during a failed import.
class GltfMaterial : public PbrMaterial {
public:
    using PbrMaterial::PbrMaterial;
    std::vector<GLuint> textures;
    ~GltfMaterial() override {
        if (glfwGetCurrentContext() && !textures.empty())
            glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
    }
};

// Preserve glTF camera projection (including fixed aspect and infinite far plane).
class GltfCamera : public Camera {
public:
    cgltf_camera source{};
    glm::mat4 getViewMatrix() const override {
        return glm::lookAt(position, position + getFront(), getUp());
    }
    glm::mat4 getProjectionMatrix(float aspect) const override {
        if (source.type == cgltf_camera_type_orthographic) {
            const auto& c = source.data.orthographic;
            return glm::ortho(-c.xmag, c.xmag, -c.ymag, c.ymag, nearPlane, farPlane);
        }
        const auto& c = source.data.perspective;
        aspect = c.has_aspect_ratio ? c.aspect_ratio : aspect;
        return c.has_zfar ? glm::perspective(c.yfov, aspect, nearPlane, farPlane) : glm::infinitePerspective(c.yfov, aspect, nearPlane);
    }
};

struct Image {
    int width = 0, height = 0;
    std::vector<unsigned char> pixels;
};

class Importer {
public:
    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data{nullptr, cgltf_free};
    std::filesystem::path directory;
    std::shared_ptr<Shader> shader;
    std::unordered_map<const cgltf_material*, std::shared_ptr<GltfMaterial>> materials;
    std::unordered_map<const cgltf_image*, Image> images;

    Importer(const std::string& path, std::shared_ptr<Shader> pbrShader)
        : directory(std::filesystem::path(path).parent_path()), shader(std::move(pbrShader)) {
        require(glfwGetCurrentContext() && glad_glGenVertexArrays && glad_glGenTextures, "A current initialized OpenGL context is required");
        require(shader != nullptr, "A PBR shader is required");
        cgltf_options options{};
        cgltf_data* parsed = nullptr;
        auto result = cgltf_parse_file(&options, path.c_str(), &parsed);
        data.reset(parsed);
        require(result == cgltf_result_success, "Cannot parse glTF/GLB file");
        for (cgltf_size i = 0; i < data->extensions_required_count; ++i) {
            std::string extension = data->extensions_required[i];
            require(extension == "KHR_lights_punctual" || extension == "KHR_mesh_quantization", "Unsupported required glTF extension");
        }
        require(cgltf_load_buffers(&options, data.get(), path.c_str()) == cgltf_result_success, "Cannot load glTF buffers");
        require(cgltf_validate(data.get()) == cgltf_result_success, "Invalid glTF data");
        if (data->animations_count)
            std::cerr << "[Warning] glTF animations are ignored; importing the static node pose\n";
    }

    const Image& image(const cgltf_image* source) {
        require(source != nullptr, "Texture has no supported image");
        auto found = images.find(source);
        if (found != images.end())
            return found->second;
        int w = 0, h = 0, channels = 0;
        unsigned char* pixels = nullptr;
        // Each engine loader explicitly selects its orientation for this thread.
        stbi_set_flip_vertically_on_load_thread(false);
        if (source->buffer_view) {
            auto* view = source->buffer_view;
            require(view->size <= INT_MAX, "Image is too large");
            auto* bytes = cgltf_buffer_view_data(view);
            require(bytes != nullptr, "Image buffer is missing");
            pixels = stbi_load_from_memory(bytes, static_cast<int>(view->size), &w, &h, &channels, 4);
        } else if (source->uri) {
            std::string uri = source->uri;
            if (uri.rfind("data:", 0) == 0) {
                auto comma = uri.find(',');
                require(comma != std::string::npos && uri.substr(0, comma).find(";base64") != std::string::npos, "Image data URI must use base64");
                std::string encoded = uri.substr(comma + 1);
                require(!encoded.empty() && encoded.size() % 4 == 0, "Invalid base64 image");
                size_t size = encoded.size() / 4 * 3;
                if (encoded.back() == '=')
                    --size;
                if (encoded[encoded.size() - 2] == '=')
                    --size;
                require(size <= INT_MAX, "Image is too large");
                void* decoded = nullptr;
                cgltf_options options{};
                require(cgltf_load_buffer_base64(&options, size, encoded.c_str(), &decoded) == cgltf_result_success, "Cannot decode image data URI");
                std::unique_ptr<void, decltype(&std::free)> owner(decoded, std::free);
                pixels = stbi_load_from_memory(static_cast<unsigned char*>(decoded), static_cast<int>(size), &w, &h, &channels, 4);
            } else {
                cgltf_decode_uri(uri.data());
                pixels = stbi_load((directory / uri.c_str()).string().c_str(), &w, &h, &channels, 4);
            }
        }
        std::unique_ptr<unsigned char, decltype(&stbi_image_free)> owner(pixels, stbi_image_free);
        require(pixels && w > 0 && h > 0, "Cannot decode glTF image");
        Image result{w, h, {}};
        result.pixels.assign(pixels, pixels + static_cast<size_t>(w) * h * 4);
        return images.emplace(source, std::move(result)).first->second;
    }

    enum class Map { Albedo, Metallic, Roughness, Occlusion, Normal };
    GLuint texture(const cgltf_texture_view& view, Map kind, glm::vec3 factor, GltfMaterial& owner) {
        if (!view.texture)
            return 0;
        require(view.texcoord == 0 && !view.has_transform, "Only TEXCOORD_0 without texture transforms is supported");
        const auto& source = image(view.texture->image);
        auto pixels = source.pixels;
        for (size_t i = 0; i < pixels.size(); i += 4) {
            if (kind == Map::Normal) {
                glm::vec3 normal = glm::vec3(pixels[i], pixels[i + 1], pixels[i + 2]) / 255.0f * 2.0f - 1.0f;
                normal.x *= view.scale;
                normal.y *= view.scale;
                normal = unit(normal, glm::vec3(0, 0, 1)) * 0.5f + 0.5f;
                for (int c = 0; c < 3; ++c)
                    pixels[i + c] = static_cast<unsigned char>(normal[c] * 255.0f + 0.5f);
                continue;
            }
            for (int c = 0; c < 3; ++c) {
                float value = pixels[i + c] / 255.0f;
                if (kind == Map::Albedo) {
                    // The engine samples albedo as linear RGB.
                    value = (value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f)) * factor[c];
                } else if (kind == Map::Metallic || kind == Map::Roughness) {
                    value = source.pixels[i + (kind == Map::Metallic ? 2 : 1)] / 255.0f * factor.x;
                } else if (kind == Map::Occlusion) {
                    value = 1.0f + view.scale * (source.pixels[i] / 255.0f - 1.0f);
                }
                pixels[i + c] = static_cast<unsigned char>(glm::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
            }
        }
        GLuint id = createTexture(pixels.data(), source.width, source.height, GL_RGBA);
        owner.textures.push_back(id);
        auto* sampler = view.texture->sampler;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler ? sampler->wrap_s : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler ? sampler->wrap_t : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler && sampler->min_filter ? sampler->min_filter : GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler && sampler->mag_filter ? sampler->mag_filter : GL_LINEAR);
        return id;
    }

    std::shared_ptr<GltfMaterial> material(const cgltf_material* source) {
        auto found = materials.find(source);
        if (found != materials.end())
            return found->second;
        glm::vec3 color(1);
        float metallic = 1, roughness = 1;
        if (source && source->has_pbr_metallic_roughness) {
            const auto& pbr = source->pbr_metallic_roughness;
            color = glm::make_vec3(pbr.base_color_factor);
            metallic = pbr.metallic_factor;
            roughness = pbr.roughness_factor;
        }
        auto result = std::make_shared<GltfMaterial>(shader, color, metallic, roughness);
        if (source) {
            if (source->alpha_mode != cgltf_alpha_mode_opaque || source->double_sided || source->unlit || source->emissive_texture.texture ||
                glm::length(glm::make_vec3(source->emissive_factor)) > 0)
                std::cerr << "[Warning] glTF alpha, double-sided, unlit and emissive properties are not supported by the PBR renderer\n";
            const auto& pbr = source->pbr_metallic_roughness;
            if (auto id = texture(pbr.base_color_texture, Map::Albedo, color, *result))
                result->setAlbedoMap(id);
            if (auto id = texture(pbr.metallic_roughness_texture, Map::Metallic, glm::vec3(metallic), *result))
                result->setMetallicMap(id);
            if (auto id = texture(pbr.metallic_roughness_texture, Map::Roughness, glm::vec3(roughness), *result))
                result->setRoughnessMap(id);
            if (auto id = texture(source->occlusion_texture, Map::Occlusion, glm::vec3(1), *result))
                result->setAoMap(id);
            if (auto id = texture(source->normal_texture, Map::Normal, glm::vec3(1), *result))
                result->setNormalMap(id);
        }
        materials.emplace(source, result);
        return result;
    }

    std::vector<float> attribute(const cgltf_primitive& primitive, cgltf_attribute_type type, cgltf_type shape, size_t count = 0) {
        const cgltf_accessor* accessor = nullptr;
        for (size_t i = 0; i < primitive.attributes_count; ++i)
            if (primitive.attributes[i].type == type && primitive.attributes[i].index == 0)
                accessor = primitive.attributes[i].data;
        if (!accessor)
            return {};
        require(accessor->type == shape && (!count || accessor->count == count), "Invalid vertex attribute shape/count");
        std::vector<float> values(accessor->count * cgltf_num_components(shape));
        require(cgltf_accessor_unpack_floats(accessor, values.data(), values.size()) == values.size(), "Cannot read vertex attribute");
        for (float value : values)
            require(std::isfinite(value), "Non-finite vertex attribute");
        return values;
    }

    void append(Model& model, const cgltf_mesh& source, const glm::mat4& transform = glm::mat4(1)) {
        const glm::mat3 linear(transform);
        const float determinant = glm::determinant(linear);
        require(std::isfinite(determinant) && std::abs(determinant) > 1e-20f, "Singular mesh node transform");
        const auto normalMatrix = glm::transpose(glm::inverse(linear));
        for (size_t p = 0; p < source.primitives_count; ++p) {
            const auto& primitive = source.primitives[p];
            require(!primitive.has_draco_mesh_compression && !primitive.targets_count, "Compressed geometry and morph targets are unsupported");
            require(primitive.type == cgltf_primitive_type_triangles || primitive.type == cgltf_primitive_type_triangle_strip ||
                        primitive.type == cgltf_primitive_type_triangle_fan,
                    "Only triangle primitives are supported");
            auto positions = attribute(primitive, cgltf_attribute_type_position, cgltf_type_vec3);
            require(!positions.empty(), "Primitive is missing POSITION");
            size_t count = positions.size() / 3;
            require(count <= UINT_MAX, "Too many vertices");
            auto normals = attribute(primitive, cgltf_attribute_type_normal, cgltf_type_vec3, count);
            auto uv = attribute(primitive, cgltf_attribute_type_texcoord, cgltf_type_vec2, count);
            auto tangents = attribute(primitive, cgltf_attribute_type_tangent, cgltf_type_vec4, count);
            if (primitive.material && uv.empty()) {
                const auto& m = *primitive.material;
                require(!m.pbr_metallic_roughness.base_color_texture.texture && !m.pbr_metallic_roughness.metallic_roughness_texture.texture &&
                            !m.normal_texture.texture && !m.occlusion_texture.texture,
                        "Textured primitive is missing TEXCOORD_0");
            }
            for (size_t a = 0; a < primitive.attributes_count; ++a) {
                require(primitive.attributes[a].type != cgltf_attribute_type_joints && primitive.attributes[a].type != cgltf_attribute_type_weights,
                        "Skinned meshes are unsupported");
                if (primitive.attributes[a].type == cgltf_attribute_type_color)
                    std::cerr << "[Warning] glTF vertex colors are ignored\n";
            }
            auto mesh = std::make_shared<Mesh>();
            mesh->vertices.resize(count);
            for (size_t i = 0; i < count; ++i) {
                auto& vertex = mesh->vertices[i];
                vertex.Position = glm::vec3(transform * glm::vec4(glm::make_vec3(&positions[i * 3]), 1));
                vertex.TexCoords = uv.empty() ? glm::vec2(0) : glm::make_vec2(&uv[i * 2]);
                vertex.Normal = normals.empty() ? glm::vec3(0) : unit(normalMatrix * glm::make_vec3(&normals[i * 3]));
                vertex.Tangent = tangents.empty() ? glm::vec3(0) : linear * glm::make_vec3(&tangents[i * 4]);
                vertex.TangentSign = tangents.empty() ? 1.0f : tangents[i * 4 + 3] * (determinant < 0 ? -1.0f : 1.0f);
            }
            std::vector<unsigned int> indices(primitive.indices ? primitive.indices->count : count);
            if (primitive.indices) {
                require(!primitive.indices->is_sparse, "Sparse index accessors are unsupported");
                require(cgltf_accessor_unpack_indices(primitive.indices, indices.data(), sizeof(unsigned int), indices.size()) == indices.size(),
                        "Cannot read indices");
            } else {
                for (size_t i = 0; i < count; ++i)
                    indices[i] = static_cast<unsigned int>(i);
            }
            for (auto index : indices)
                require(index < count, "Index outside vertex array");
            require(indices.size() >= 3, "Empty triangle primitive");
            if (primitive.type == cgltf_primitive_type_triangles) {
                require(indices.size() % 3 == 0, "Incomplete triangle");
                mesh->indices = std::move(indices);
            } else {
                for (size_t i = 2; i < indices.size(); ++i) {
                    unsigned int a = primitive.type == cgltf_primitive_type_triangle_fan ? indices[0] : indices[i - 2];
                    unsigned int b = indices[i - 1], c = indices[i];
                    if (primitive.type == cgltf_primitive_type_triangle_strip && i % 2)
                        std::swap(a, b);
                    mesh->indices.insert(mesh->indices.end(), {a, b, c});
                }
            }
            require(mesh->indices.size() <= UINT_MAX, "Too many indices");
            if (determinant < 0)
                for (size_t i = 0; i < mesh->indices.size(); i += 3)
                    std::swap(mesh->indices[i + 1], mesh->indices[i + 2]);
            // Missing normals use flat shading, as required by glTF.
            if (normals.empty()) {
                std::vector<Vertex> flat;
                flat.reserve(mesh->indices.size());
                for (size_t i = 0; i < mesh->indices.size(); i += 3) {
                    auto a = mesh->vertices[mesh->indices[i]], b = mesh->vertices[mesh->indices[i + 1]], c = mesh->vertices[mesh->indices[i + 2]];
                    a.Normal = b.Normal = c.Normal = unit(glm::cross(b.Position - a.Position, c.Position - a.Position));
                    flat.insert(flat.end(), {a, b, c});
                }
                mesh->vertices = std::move(flat);
                for (size_t i = 0; i < mesh->indices.size(); ++i)
                    mesh->indices[i] = static_cast<unsigned int>(i);
            }
            std::vector<glm::vec3> bitangents(mesh->vertices.size(), glm::vec3(0));
            if (tangents.empty()) {
                for (size_t i = 0; i < mesh->indices.size(); i += 3) {
                    auto ia = mesh->indices[i], ib = mesh->indices[i + 1], ic = mesh->indices[i + 2];
                    auto& a = mesh->vertices[ia];
                    auto& b = mesh->vertices[ib];
                    auto& c = mesh->vertices[ic];
                    auto e1 = b.Position - a.Position, e2 = c.Position - a.Position;
                    auto d1 = b.TexCoords - a.TexCoords, d2 = c.TexCoords - a.TexCoords;
                    float det = d1.x * d2.y - d1.y * d2.x;
                    if (std::abs(det) < 1e-10f)
                        continue;
                    auto t = (e1 * d2.y - e2 * d1.y) / det;
                    auto bt = (e2 * d1.x - e1 * d2.x) / det;
                    for (auto index : {ia, ib, ic}) {
                        mesh->vertices[index].Tangent += t;
                        bitangents[index] += bt;
                    }
                }
            }
            for (size_t i = 0; i < mesh->vertices.size(); ++i) {
                auto& v = mesh->vertices[i];
                auto fallback = unit(glm::cross(std::abs(v.Normal.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0), v.Normal));
                v.Tangent = unit(v.Tangent - v.Normal * glm::dot(v.Normal, v.Tangent), fallback);
                if (tangents.empty())
                    v.TangentSign = glm::dot(glm::cross(v.Normal, v.Tangent), bitangents[i]) < 0 ? -1 : 1;
            }
            auto mat = material(primitive.material);
            mesh->setup();
            model.subMeshes.emplace_back(std::move(mesh), std::move(mat));
        }
        model.calculateBounds();
    }
};

glm::quat orientation(const glm::mat4& world) {
    auto front = unit(-glm::vec3(world[2]), glm::vec3(0, 0, -1));
    auto up = unit(glm::vec3(world[1]));
    require(std::abs(glm::dot(front, up)) < 0.9999f, "Degenerate camera/light transform");
    return glm::quatLookAt(front, up);
}
} // namespace

std::shared_ptr<Model> loadModelGLTF(const std::string& path, std::shared_ptr<Shader> shader) {
    try {
        Importer importer(path, std::move(shader));
        auto model = std::make_shared<Model>();
        for (size_t i = 0; i < importer.data->meshes_count; ++i)
            importer.append(*model, importer.data->meshes[i]);
        require(!model->subMeshes.empty(), "glTF has no meshes");
        return model;
    } catch (const std::exception& e) {
        std::cerr << "[Error] Cannot load glTF model '" << path << "': " << e.what() << '\n';
        return nullptr;
    }
}

bool Scene::loadGLTF(const std::string& path, int sceneIndex) {
    try {
        Importer importer(path, resourceManager.getShader("pbrShader"));
        auto& data = *importer.data;
        require(sceneIndex >= -1 && (sceneIndex == -1 || static_cast<size_t>(sceneIndex) < data.scenes_count), "Scene index out of range");
        const cgltf_scene* selected = sceneIndex >= 0 ? &data.scenes[sceneIndex] : data.scene;
        if (!selected && data.scenes_count)
            selected = &data.scenes[0];
        struct Entry {
            const cgltf_node* node;
            glm::mat4 parent;
        };
        std::vector<Entry> pending;
        if (selected) {
            for (size_t i = selected->nodes_count; i > 0; --i)
                pending.push_back({selected->nodes[i - 1], glm::mat4(1)});
        } else {
            for (size_t i = data.nodes_count; i > 0; --i)
                if (!data.nodes[i - 1].parent)
                    pending.push_back({&data.nodes[i - 1], glm::mat4(1)});
        }
        std::vector<std::shared_ptr<Object>> objects;
        std::vector<std::shared_ptr<Light>> lights;
        std::shared_ptr<Camera> importedCamera;
        std::unordered_set<const cgltf_node*> visited;
        while (!pending.empty()) {
            auto entry = pending.back();
            pending.pop_back();
            auto* node = entry.node;
            require(visited.insert(node).second, "Repeated or cyclic scene node");
            require(!node->skin && !node->has_mesh_gpu_instancing, "Skinning and GPU instancing extensions are unsupported");
            glm::mat4 local;
            cgltf_node_transform_local(node, glm::value_ptr(local));
            auto world = entry.parent * local;
            for (int c = 0; c < 4; ++c)
                for (int r = 0; r < 4; ++r)
                    require(std::isfinite(world[c][r]), "Non-finite node transform");
            if (node->mesh) {
                auto model = std::make_shared<Model>();
                auto linear = world;
                linear[3] = glm::vec4(0, 0, 0, 1);
                importer.append(*model, *node->mesh, linear);
                auto object = std::make_shared<Object>(model);
                object->position = glm::vec3(world[3]);
                if (node->name)
                    object->setGroup(node->name);
                objects.push_back(std::move(object));
            }
            if (node->camera && !importedCamera) {
                auto cam = std::make_shared<GltfCamera>();
                cam->source.type = node->camera->type;
                cam->source.data = node->camera->data;
                cam->position = glm::vec3(world[3]);
                cam->rotation = orientation(world);
                if (cam->source.type == cgltf_camera_type_perspective) {
                    cam->nearPlane = cam->source.data.perspective.znear;
                    cam->farPlane =
                        cam->source.data.perspective.has_zfar ? cam->source.data.perspective.zfar : std::numeric_limits<float>::infinity();
                } else {
                    cam->nearPlane = cam->source.data.orthographic.znear;
                    cam->farPlane = cam->source.data.orthographic.zfar;
                }
                importedCamera = cam;
            }
            if (node->light) {
                const auto& source = *node->light;
                std::shared_ptr<Light> light;
                if (source.type == cgltf_light_type_point)
                    light = std::make_shared<PbrPointLight>();
                else if (source.type == cgltf_light_type_directional)
                    light = std::make_shared<DirLight>();
                else
                    throw std::runtime_error("Spot lights are unsupported");
                light->position = glm::vec3(world[3]);
                light->rotation = orientation(world);
                light->color = glm::make_vec3(source.color);
                light->intensity = source.intensity;
                if (auto directional = std::dynamic_pointer_cast<DirLight>(light)) {
                    directional->ambient = glm::vec3(0);
                    directional->diffuse = directional->specular = light->color * light->intensity;
                }
                if (source.range > 0)
                    std::cerr << "[Warning] glTF light range is ignored by the renderer\n";
                lights.push_back(std::move(light));
            }
            for (size_t i = node->children_count; i > 0; --i)
                pending.push_back({node->children[i - 1], world});
        }
        // Commit only after the entire selected scene has imported successfully.
        clear();
        for (auto& object : objects)
            objectManager.registerObject(std::move(object));
        for (auto& light : lights)
            lightManager.registerLight(std::move(light));
        if (importedCamera)
            setCamera(std::move(importedCamera));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Error] Cannot load glTF scene '" << path << "': " << e.what() << '\n';
        return false;
    }
}
} // namespace knot
