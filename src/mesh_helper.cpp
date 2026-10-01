#include <glad/gl.h>
#include <knot/resources.h>
#include <knot/utility/mesh_helper.h>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include <cmath>
#include <filesystem>
#include <map>
#include <unordered_map>

namespace knot {
namespace {

std::string getObjDirectory(const std::string& filePath) {
    return std::filesystem::path(filePath).parent_path().string();
}

Vertex makeVertexFromObjIndex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index) {
    Vertex vertex{};

    vertex.Position = glm::vec3(attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2]);

    if (index.texcoord_index >= 0) {
        vertex.TexCoords = glm::vec2(attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
    } else {
        vertex.TexCoords = glm::vec2(0.0f);
    }

    if (index.normal_index >= 0) {
        vertex.Normal = glm::vec3(attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1],
                                  attrib.normals[3 * index.normal_index + 2]);
    } else {
        vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    return vertex;
}

} // namespace

void calculateMeshTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    for (auto& v : vertices) {
        v.Tangent = glm::vec3(0.0f);
    }

    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;
        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;
    }

    for (auto& v : vertices) {
        v.Tangent = glm::normalize(v.Tangent - v.Normal * glm::dot(v.Normal, v.Tangent));
    }
}

std::shared_ptr<Mesh> createCube() {
    auto mesh = std::make_shared<Mesh>();
    mesh->vertices = {// Front (+Z)
                      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      // Back (-Z)
                      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
                      // Top (+Y)
                      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      // Bottom (-Y)
                      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                      // Left (-X)
                      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                      // Right (+X)
                      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}};

    mesh->indices = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Left
        20, 21, 22, 22, 23, 20  // Right
    };

    mesh->setup();
    return mesh;
}
std::shared_ptr<Mesh> createSphere(int sectors, int stacks) {
    auto mesh = std::make_shared<Mesh>();
    const float PI = 3.14159265359f;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2 - i * (PI / stacks);
        float xy = cosf(stackAngle);
        float z = sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * (2.0f * PI / sectors);
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            glm::vec3 pos(x * 0.5f, y * 0.5f, z * 0.5f);
            glm::vec3 normal(x, y, z);
            glm::vec2 uv((float)j / sectors, (float)i / stacks);

            glm::vec3 tangent(-xy * sinf(sectorAngle), xy * cosf(sectorAngle), 0.0f);

            if (glm::length(tangent) < 0.0001f) {
                tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            } else {
                tangent = glm::normalize(tangent);
            }

            mesh->vertices.push_back({pos, uv, normal, tangent});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = (i * (sectors + 1)) + j;
            int second = first + sectors + 1;

            mesh->indices.push_back(first);
            mesh->indices.push_back(second);
            mesh->indices.push_back(first + 1);

            mesh->indices.push_back(second);
            mesh->indices.push_back(second + 1);
            mesh->indices.push_back(first + 1);
        }
    }

    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> createPlane(float width, float height) {
    auto mesh = std::make_shared<Mesh>();
    float hw = width * 0.5f, hh = height * 0.5f;

    mesh->vertices = {{{-hw, -hh, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{hw, -hh, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{hw, hh, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
                      {{-hw, hh, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}};
    mesh->indices = {0, 1, 2, 2, 3, 0};
    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> createRegularPolygon(int sectors, float radius) {
    if (sectors < 3)
        return nullptr;

    auto mesh = std::make_shared<Mesh>();
    const float PI = 3.14159265359f;

    mesh->vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});

    for (int i = 0; i < sectors; ++i) {
        float angle = i * (2.0f * PI / sectors);
        float x = cosf(angle) * radius;
        float y = sinf(angle) * radius;

        mesh->vertices.push_back({{x, y, 0.0f}, {0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle)}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}});
    }

    for (int i = 1; i <= sectors; ++i) {
        mesh->indices.push_back(0);
        mesh->indices.push_back((i == sectors) ? 1 : i + 1);
        mesh->indices.push_back(i);
    }

    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> createMeshFromVertices(const std::vector<glm::vec3>& positions) {
    if (positions.size() < 3)
        return nullptr;

    auto mesh = std::make_shared<Mesh>();

    glm::vec3 center(0.0f);
    for (const auto& pos : positions)
        center += pos;
    center /= static_cast<float>(positions.size());

    mesh->vertices.push_back({center, {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}});

    float maxDist = 0.001f;
    for (const auto& pos : positions) {
        maxDist = glm::max(maxDist, glm::distance(center, pos));
    }

    for (const auto& pos : positions) {
        glm::vec3 dir = pos - center;
        glm::vec2 uv = glm::vec2(dir.x, dir.y) / (maxDist * 2.0f) + glm::vec2(0.5f);

        mesh->vertices.push_back({pos, uv, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}});
    }

    int count = (int)positions.size();
    for (int i = 1; i <= count; ++i) {
        mesh->indices.push_back(0);
        mesh->indices.push_back((i == count) ? 1 : i + 1);
        mesh->indices.push_back(i);
    }

    calculateMeshTangents(mesh->vertices, mesh->indices);

    mesh->setup();
    return mesh;
}

std::shared_ptr<Mesh> loadModelOBJ(const std::string& filePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    const std::string objDir = getObjDirectory(filePath);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), objDir.c_str())) {
        std::cerr << "[Error] Failed to load the OBJ File: " << warn << err << std::endl;
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();

    struct IndexTuple {
        int v_idx, vt_idx, vn_idx;
        bool operator<(const IndexTuple& other) const {
            if (v_idx != other.v_idx)
                return v_idx < other.v_idx;
            if (vt_idx != other.vt_idx)
                return vt_idx < other.vt_idx;
            return vn_idx < other.vn_idx;
        }
    };
    std::map<IndexTuple, unsigned int> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            IndexTuple idxTuple = {index.vertex_index, index.texcoord_index, index.normal_index};

            if (uniqueVertices.count(idxTuple) == 0) {
                Vertex vertex = makeVertexFromObjIndex(attrib, index);

                uniqueVertices[idxTuple] = static_cast<unsigned int>(mesh->vertices.size());
                mesh->vertices.push_back(vertex);
            }

            mesh->indices.push_back(uniqueVertices[idxTuple]);
        }
    }

    if (!mesh->indices.empty()) {
        calculateMeshTangents(mesh->vertices, mesh->indices);
    }

    mesh->indexCount = static_cast<unsigned int>(mesh->indices.size());
    mesh->setup();

    return mesh;
}

std::shared_ptr<Model> loadModelOBJWithMTL(const std::string& filePath, std::shared_ptr<Shader> pbrShader) {
    if (!pbrShader) {
        std::cerr << "[Error] A PBR shader is required to load OBJ materials" << std::endl;
        return nullptr;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    const std::string objDir = getObjDirectory(filePath);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), objDir.c_str())) {
        std::cerr << "[Error] Failed to load the OBJ File: " << warn << err << std::endl;
        return nullptr;
    }

    struct IndexTuple {
        int v_idx, vt_idx, vn_idx;
        bool operator<(const IndexTuple& other) const {
            if (v_idx != other.v_idx)
                return v_idx < other.v_idx;
            if (vt_idx != other.vt_idx)
                return vt_idx < other.vt_idx;
            return vn_idx < other.vn_idx;
        }
    };

    struct MaterialGroup {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::map<IndexTuple, unsigned int> uniqueVertices;
    };

    std::map<int, MaterialGroup> groups;

    for (const auto& shape : shapes) {
        size_t indexOffset = 0;
        size_t faceIndex = 0;

        for (const unsigned char faceVertexCount : shape.mesh.num_face_vertices) {
            int materialId = -1;

            if (faceIndex < shape.mesh.material_ids.size()) {
                materialId = shape.mesh.material_ids[faceIndex];
            }
            ++faceIndex;

            MaterialGroup& group = groups[materialId];

            for (unsigned char v = 0; v < faceVertexCount; ++v) {
                const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

                IndexTuple idxTuple = {idx.vertex_index, idx.texcoord_index, idx.normal_index};

                auto it = group.uniqueVertices.find(idxTuple);
                if (it == group.uniqueVertices.end()) {
                    it = group.uniqueVertices.emplace(idxTuple, static_cast<unsigned int>(group.vertices.size())).first;
                    group.vertices.push_back(makeVertexFromObjIndex(attrib, idx));
                }

                group.indices.push_back(it->second);
            }

            indexOffset += faceVertexCount;
        }
    }

    std::unordered_map<std::string, unsigned int> textureCache;

    auto getTexture = [&objDir, &textureCache](const std::string& texName) -> unsigned int {
        if (texName.empty())
            return 0;

        std::filesystem::path path(texName);

        if (!path.is_absolute()) {
            path = std::filesystem::path(objDir) / path;
        }

        const std::string pathStr = path.generic_string();

        auto it = textureCache.find(pathStr);
        if (it != textureCache.end())
            return it->second;

        if (!std::filesystem::exists(path)) {
            std::cout << "[Warning] MTL texture not found: " << pathStr << std::endl;
            textureCache.emplace(pathStr, 0);
            return 0;
        }

        const unsigned int textureId = loadTextureFromFile(pathStr);
        textureCache.emplace(pathStr, textureId);
        return textureId;
    };

    auto createMaterial = [&materials, &getTexture, pbrShader](int materialId) -> std::shared_ptr<PbrMaterial> {
        glm::vec3 albedoColor(1.0f);
        float metallicFactor = 0.0f;
        float roughnessFactor = 0.5f;

        unsigned int albedoMap = 0;
        unsigned int metallicMap = 0;
        unsigned int roughnessMap = 0;
        unsigned int normalMap = 0;

        if (materialId >= 0 && materialId < static_cast<int>(materials.size())) {
            const tinyobj::material_t& mtl = materials[materialId];

            albedoColor = glm::vec3(mtl.diffuse[0], mtl.diffuse[1], mtl.diffuse[2]);

            if (!mtl.diffuse_texname.empty()) {
                albedoMap = getTexture(mtl.diffuse_texname);
            }

            if (!mtl.metallic_texname.empty()) {
                metallicMap = getTexture(mtl.metallic_texname);
            } else if (mtl.metallic > 0.0f) {
                metallicFactor = glm::clamp(mtl.metallic, 0.0f, 1.0f);
            }

            if (!mtl.roughness_texname.empty()) {
                roughnessMap = getTexture(mtl.roughness_texname);
            } else if (mtl.roughness > 0.0f) {
                roughnessFactor = glm::clamp(mtl.roughness, 0.0f, 1.0f);
            } else if (mtl.shininess > 0.0f) {
                roughnessFactor = glm::clamp(sqrtf(2.0f / (mtl.shininess + 2.0f)), 0.05f, 1.0f);
            } else {
                roughnessFactor = 1.0f;
            }

            normalMap = getTexture(mtl.normal_texname);

            if (normalMap == 0) {
                normalMap = getTexture(mtl.bump_texname);
            }
        }

        return std::make_shared<PbrMaterial>(pbrShader, albedoColor, metallicFactor, roughnessFactor, 1.0f, albedoMap, metallicMap, roughnessMap, 0,
                                             normalMap);
    };

    auto model = std::make_shared<Model>();

    for (auto& [materialId, group] : groups) {
        if (group.indices.empty())
            continue;

        auto mesh = std::make_shared<Mesh>();
        mesh->vertices = std::move(group.vertices);
        mesh->indices = std::move(group.indices);

        calculateMeshTangents(mesh->vertices, mesh->indices);

        mesh->indexCount = static_cast<unsigned int>(mesh->indices.size());
        mesh->setup();

        model->subMeshes.emplace_back(std::move(mesh), createMaterial(materialId));
    }

    if (model->subMeshes.empty()) {
        return nullptr;
    }

    model->calculateBounds();
    return model;
}

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

// stb_image.h is already implemented in texture.cpp; include without the
// implementation guard so we can call stbi_load_from_memory for embedded images.
#include <stb/stb_image.h>

#include <knot/utility/texture.h>

namespace {

/** @brief Reads a typed accessor into a flat float vector. */
std::vector<float> cgltfReadFloats(const cgltf_accessor* accessor) {
    if (!accessor)
        return {};

    const size_t count = accessor->count;
    const size_t components = cgltf_num_components(accessor->type);
    std::vector<float> out(count * components);

    for (size_t i = 0; i < count; ++i) {
        cgltf_accessor_read_float(accessor, i, out.data() + i * components, components);
    }

    return out;
}

/** @brief Reads an index accessor into a flat uint32 vector. */
std::vector<unsigned int> cgltfReadIndices(const cgltf_accessor* accessor) {
    if (!accessor)
        return {};

    const size_t count = accessor->count;
    std::vector<unsigned int> out(count);

    for (size_t i = 0; i < count; ++i) {
        cgltf_uint value = 0;
        cgltf_accessor_read_uint(accessor, i, &value, 1);
        out[i] = static_cast<unsigned int>(value);
    }

    return out;
}

/** @brief Loads a glTF texture, optionally extracting a packed channel and applying its factor. */
unsigned int cgltfLoadTexture(const cgltf_texture_view& view, const std::string& gltfDir, std::unordered_map<std::string, unsigned int>& cache,
                              int channel = -1, glm::vec3 factor = glm::vec3(1.0f)) {
    if (!view.texture || !view.texture->image)
        return 0;

    const cgltf_image* image = view.texture->image;
    const std::string key = std::to_string(reinterpret_cast<uintptr_t>(image)) + ":" + std::to_string(channel) + ":" + std::to_string(factor.x) +
                            ":" + std::to_string(factor.y) + ":" + std::to_string(factor.z);
    auto it = cache.find(key);
    if (it != cache.end())
        return it->second;

    // glTF UVs use the image's top-left origin. Do not inherit the HDR/OBJ loader's flip.
    stbi_set_flip_vertically_on_load(false);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = nullptr;
    if (image->buffer_view) {
        const cgltf_buffer_view* bv = image->buffer_view;
        const unsigned char* data = static_cast<const unsigned char*>(bv->buffer->data) + bv->offset;
        pixels = stbi_load_from_memory(data, static_cast<int>(bv->size), &w, &h, &channels, 4);
    } else if (image->uri) {
        std::filesystem::path texPath(image->uri);
        if (!texPath.is_absolute())
            texPath = std::filesystem::path(gltfDir) / texPath;
        pixels = stbi_load(texPath.string().c_str(), &w, &h, &channels, 4);
    }

    if (!pixels) {
        std::cerr << "[Warning] GLTF: failed to decode texture: " << (image->uri ? image->uri : "embedded image") << "\n";
        cache.emplace(key, 0u);
        return 0;
    }

    // The engine samples separate RGB/red maps, while glTF packs roughness in G and metallic in B.
    for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i) {
        unsigned char* pixel = pixels + i * 4;
        const glm::vec3 rgb = channel >= 0 ? glm::vec3(pixel[channel]) : glm::vec3(pixel[0], pixel[1], pixel[2]);
        const glm::vec3 scaled = glm::clamp(rgb * factor, glm::vec3(0.0f), glm::vec3(255.0f));
        for (int c = 0; c < 3; ++c)
            pixel[c] = static_cast<unsigned char>(scaled[c]);
    }
    const unsigned int texId = knot::createTexture(pixels, w, h, GL_RGBA);
    stbi_image_free(pixels);
    cache.emplace(key, texId);
    return texId;
}

/** @brief Converts a single cgltf_primitive into a knot::Mesh. */
std::shared_ptr<knot::Mesh> cgltfPrimitiveToMesh(const cgltf_primitive& prim) {
    if (prim.type != cgltf_primitive_type_triangles)
        return nullptr;

    const cgltf_accessor* posAcc = nullptr;
    const cgltf_accessor* normAcc = nullptr;
    const cgltf_accessor* uvAcc = nullptr;
    const cgltf_accessor* tanAcc = nullptr;

    for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai) {
        const cgltf_attribute& attr = prim.attributes[ai];
        switch (attr.type) {
        case cgltf_attribute_type_position:
            posAcc = attr.data;
            break;
        case cgltf_attribute_type_normal:
            normAcc = attr.data;
            break;
        case cgltf_attribute_type_texcoord:
            if (attr.index == 0)
                uvAcc = attr.data;
            break;
        case cgltf_attribute_type_tangent:
            tanAcc = attr.data;
            break;
        default:
            break;
        }
    }

    if (!posAcc)
        return nullptr;

    const size_t vertexCount = posAcc->count;

    std::vector<float> positions = cgltfReadFloats(posAcc);
    std::vector<float> normals = cgltfReadFloats(normAcc);
    std::vector<float> uvs = cgltfReadFloats(uvAcc);
    std::vector<float> tangents = cgltfReadFloats(tanAcc);

    auto mesh = std::make_shared<knot::Mesh>();
    mesh->vertices.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        knot::Vertex& v = mesh->vertices[i];

        v.Position = glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);

        if (normals.size() >= (i + 1) * 3) {
            v.Normal = glm::vec3(normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]);
        } else {
            v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (uvs.size() >= (i + 1) * 2) {
            v.TexCoords = glm::vec2(uvs[i * 2 + 0], uvs[i * 2 + 1]);
        } else {
            v.TexCoords = glm::vec2(0.0f);
        }

        // glTF tangents are vec4 (xyz = tangent, w = handedness)
        if (tangents.size() >= (i + 1) * 4) {
            v.Tangent = glm::vec3(tangents[i * 4 + 0], tangents[i * 4 + 1], tangents[i * 4 + 2]);
        } else {
            v.Tangent = glm::vec3(0.0f);
        }
    }

    mesh->indices = cgltfReadIndices(prim.indices);

    if (tangents.empty() && !mesh->indices.empty()) {
        knot::calculateMeshTangents(mesh->vertices, mesh->indices);
    }

    mesh->indexCount = static_cast<unsigned int>(mesh->indices.size());
    mesh->setup();

    return mesh;
}

/** @brief Creates a PbrMaterial from a cgltf_material. */
std::shared_ptr<knot::PbrMaterial> cgltfMakeMaterial(const cgltf_material* mat, const std::string& gltfDir,
                                                     std::unordered_map<std::string, unsigned int>& texCache, std::shared_ptr<knot::Shader> shader) {
    glm::vec3 albedoColor(1.0f);
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    unsigned int albedoMap = 0;
    unsigned int metallicMap = 0;
    unsigned int roughnessMap = 0;
    unsigned int normalMap = 0;
    unsigned int aoMap = 0;

    if (mat && mat->has_pbr_metallic_roughness) {
        const cgltf_pbr_metallic_roughness& pbr = mat->pbr_metallic_roughness;
        albedoColor = glm::vec3(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]);
        metallicFactor = pbr.metallic_factor;
        roughnessFactor = pbr.roughness_factor;
        albedoMap = cgltfLoadTexture(pbr.base_color_texture, gltfDir, texCache, -1, albedoColor);
        metallicMap = cgltfLoadTexture(pbr.metallic_roughness_texture, gltfDir, texCache, 2, glm::vec3(metallicFactor));
        roughnessMap = cgltfLoadTexture(pbr.metallic_roughness_texture, gltfDir, texCache, 1, glm::vec3(roughnessFactor));
    }

    if (mat) {
        normalMap = cgltfLoadTexture(mat->normal_texture, gltfDir, texCache);
        aoMap = cgltfLoadTexture(mat->occlusion_texture, gltfDir, texCache);
    }

    return std::make_shared<knot::PbrMaterial>(shader, albedoColor, metallicFactor, roughnessFactor, 1.0f, albedoMap, metallicMap, roughnessMap,
                                               aoMap, normalMap);
}

} // anonymous namespace

std::shared_ptr<Model> loadModelGLTF(const std::string& filePath, std::shared_ptr<Shader> pbrShader) {
    if (!pbrShader) {
        std::cerr << "[Error] GLTF: A PBR shader is required\n";
        return nullptr;
    }

    cgltf_options options{};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, filePath.c_str(), &data) != cgltf_result_success) {
        std::cerr << "[Error] GLTF: Failed to parse file: " << filePath << "\n";
        return nullptr;
    }

    if (cgltf_load_buffers(&options, data, filePath.c_str()) != cgltf_result_success) {
        std::cerr << "[Error] GLTF: Failed to load buffers: " << filePath << "\n";
        cgltf_free(data);
        return nullptr;
    }

    const std::string gltfDir = std::filesystem::path(filePath).parent_path().string();
    std::unordered_map<std::string, unsigned int> texCache;

    auto model = std::make_shared<Model>();

    for (cgltf_size mi = 0; mi < data->meshes_count; ++mi) {
        const cgltf_mesh& gltfMesh = data->meshes[mi];

        for (cgltf_size pi = 0; pi < gltfMesh.primitives_count; ++pi) {
            auto mesh = cgltfPrimitiveToMesh(gltfMesh.primitives[pi]);
            if (!mesh)
                continue;

            auto material = cgltfMakeMaterial(gltfMesh.primitives[pi].material, gltfDir, texCache, pbrShader);
            model->subMeshes.emplace_back(std::move(mesh), std::move(material));
        }
    }

    cgltf_free(data);

    if (model->subMeshes.empty()) {
        std::cerr << "[Error] GLTF: No triangle primitives found in: " << filePath << "\n";
        return nullptr;
    }

    model->calculateBounds();
    return model;
}

std::vector<std::shared_ptr<Object>> loadSceneGLTF(const std::string& filePath, std::shared_ptr<Shader> pbrShader) {
    if (!pbrShader) {
        std::cerr << "[Error] GLTF: A PBR shader is required\n";
        return {};
    }

    cgltf_options options{};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, filePath.c_str(), &data) != cgltf_result_success) {
        std::cerr << "[Error] GLTF: Failed to parse file: " << filePath << "\n";
        return {};
    }

    if (cgltf_load_buffers(&options, data, filePath.c_str()) != cgltf_result_success) {
        std::cerr << "[Error] GLTF: Failed to load buffers: " << filePath << "\n";
        cgltf_free(data);
        return {};
    }

    const std::string gltfDir = std::filesystem::path(filePath).parent_path().string();
    std::unordered_map<std::string, unsigned int> texCache;

    std::vector<std::shared_ptr<Object>> objects;

    const cgltf_scene* scene = data->scene;
    if (!scene && data->scenes_count > 0)
        scene = &data->scenes[0];

    if (!scene) {
        std::cerr << "[Warning] GLTF: No scenes found in: " << filePath << "\n";
        cgltf_free(data);
        return {};
    }

    // Iterative DFS traversal with accumulated parent world transform
    struct NodeEntry {
        const cgltf_node* node;
        glm::mat4 parentWorld;
    };

    std::vector<NodeEntry> stack;
    stack.reserve(64);

    for (cgltf_size ri = 0; ri < scene->nodes_count; ++ri) {
        stack.push_back({scene->nodes[ri], glm::mat4(1.0f)});
    }

    while (!stack.empty()) {
        auto [node, parentWorld] = stack.back();
        stack.pop_back();

        if (!node)
            continue;

        // Build local matrix
        glm::mat4 localMatrix(1.0f);
        if (node->has_matrix) {
            std::memcpy(&localMatrix[0][0], node->matrix, sizeof(float) * 16);
        } else {
            glm::vec3 t(0.0f), s(1.0f);
            glm::quat r(1.0f, 0.0f, 0.0f, 0.0f);

            if (node->has_translation)
                t = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
            if (node->has_rotation)
                r = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
            if (node->has_scale)
                s = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);

            localMatrix = glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
        }

        const glm::mat4 worldMatrix = parentWorld * localMatrix;

        // Push children first (reversed so left-to-right traversal order is preserved)
        for (cgltf_size ci = node->children_count; ci > 0; --ci) {
            stack.push_back({node->children[ci - 1], worldMatrix});
        }

        if (!node->mesh)
            continue;

        const cgltf_mesh& gltfMesh = *node->mesh;
        auto model = std::make_shared<Model>();

        for (cgltf_size pi = 0; pi < gltfMesh.primitives_count; ++pi) {
            auto mesh = cgltfPrimitiveToMesh(gltfMesh.primitives[pi]);
            if (!mesh)
                continue;

            auto material = cgltfMakeMaterial(gltfMesh.primitives[pi].material, gltfDir, texCache, pbrShader);
            model->subMeshes.emplace_back(std::move(mesh), std::move(material));
        }

        if (model->subMeshes.empty())
            continue;

        model->calculateBounds();

        // Decompose world matrix into T / R / S (approximate for TRS nodes)
        auto obj = std::make_shared<Object>(std::move(model));

        obj->position = glm::vec3(worldMatrix[3]);

        glm::vec3 col0 = glm::vec3(worldMatrix[0]);
        glm::vec3 col1 = glm::vec3(worldMatrix[1]);
        glm::vec3 col2 = glm::vec3(worldMatrix[2]);

        obj->scale = glm::vec3(glm::length(col0), glm::length(col1), glm::length(col2));

        if (obj->scale.x > 0.0f && obj->scale.y > 0.0f && obj->scale.z > 0.0f) {
            glm::mat3 rotMat(col0 / obj->scale.x, col1 / obj->scale.y, col2 / obj->scale.z);
            obj->rotation = glm::quat_cast(rotMat);
        }

        if (node->name)
            obj->setGroup(node->name);

        objects.push_back(std::move(obj));
    }

    cgltf_free(data);

    if (objects.empty())
        std::cerr << "[Warning] GLTF: No mesh nodes found in scene: " << filePath << "\n";

    return objects;
}

} // namespace knot
