#include <knot/resources.h>
#include <knot/camera.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <limits>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GeneratedShaders.h"

namespace knot {

unsigned int createSolidColorTexture(glm::vec3 color);

namespace {

#ifdef NOTENGINE_ASSET_ROOT
std::string assetRoot = []() {
    std::string root = NOTENGINE_ASSET_ROOT;
    if (!root.empty() && root.back() != '/') {
        root += '/';
    }
    return root;
}();
#else
std::string assetRoot = "";
#endif

size_t uniformSize(sg_uniform_type type) {
    switch (type) {
    case SG_UNIFORMTYPE_FLOAT:
    case SG_UNIFORMTYPE_INT:
        return 4;
    case SG_UNIFORMTYPE_FLOAT2:
    case SG_UNIFORMTYPE_INT2:
        return 8;
    case SG_UNIFORMTYPE_FLOAT3:
    case SG_UNIFORMTYPE_INT3:
        return 12;
    case SG_UNIFORMTYPE_FLOAT4:
    case SG_UNIFORMTYPE_INT4:
        return 16;
    case SG_UNIFORMTYPE_MAT4:
        return 64;
    default:
        return 0;
    }
}

// Keep reflection next to the shader resource code, without a runtime GLSL parser.
void describeShader(ShaderSource& source) {
    auto& desc = source.interface;
    const auto vertex = std::filesystem::path(source.vertexPath).filename().string();
    const auto fragment = std::filesystem::path(source.fragmentPath).filename().string();
    using U = std::pair<const char*, sg_uniform_type>;
    auto block = [&](int slot, std::initializer_list<U> members) {
        auto& b = desc.uniform_blocks[slot];
        b.stage = slot == 0 ? SG_SHADERSTAGE_VERTEX : SG_SHADERSTAGE_FRAGMENT;
        b.layout = SG_UNIFORMLAYOUT_NATIVE;
        int i = 0;
        for (const auto& member : members) {
            b.glsl_uniforms[i++] = {member.second, 1, member.first};
            b.size += static_cast<uint32_t>(uniformSize(member.second));
        }
    };
    desc.attrs[0].base_type = SG_SHADERATTRBASETYPE_FLOAT;
    if (vertex == "alpha.vert") {
        for (int i = 1; i < 8; ++i)
            desc.attrs[i].base_type = SG_SHADERATTRBASETYPE_FLOAT;
        block(0, {{"model", SG_UNIFORMTYPE_MAT4},
                  {"view", SG_UNIFORMTYPE_MAT4},
                  {"projection", SG_UNIFORMTYPE_MAT4},
                  {"lightSpaceMatrix", SG_UNIFORMTYPE_MAT4},
                  {"isInstanced", SG_UNIFORMTYPE_INT}});
        if (fragment == "alpha.frag") {
            auto& b = desc.uniform_blocks[0];
            b.glsl_uniforms[3] = b.glsl_uniforms[4];
            b.glsl_uniforms[4] = {};
            b.size -= 64;
        }
    } else if (vertex == "dir_shadow.vert" || vertex == "point_shadow.vert") {
        for (int i = 4; i < 8; ++i)
            desc.attrs[i].base_type = SG_SHADERATTRBASETYPE_FLOAT;
        block(0, {{"model", SG_UNIFORMTYPE_MAT4}, {"lightSpaceMatrix", SG_UNIFORMTYPE_MAT4}, {"isInstanced", SG_UNIFORMTYPE_INT}});
    } else if (vertex == "brdf.vert") {
        desc.attrs[1].base_type = SG_SHADERATTRBASETYPE_FLOAT;
    } else {
        block(0, {{"projection", SG_UNIFORMTYPE_MAT4}, {"view", SG_UNIFORMTYPE_MAT4}});
    }
    int textureSlot = 0;
    auto texture = [&](const char* name, sg_image_type type, bool depth = false) {
        const int slot = textureSlot++;
        desc.views[slot].texture.stage = SG_SHADERSTAGE_FRAGMENT;
        desc.views[slot].texture.image_type = type;
        desc.views[slot].texture.sample_type = depth ? SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT : SG_IMAGESAMPLETYPE_FLOAT;
        desc.samplers[slot].stage = SG_SHADERSTAGE_FRAGMENT;
        desc.samplers[slot].sampler_type = depth ? SG_SAMPLERTYPE_NONFILTERING : SG_SAMPLERTYPE_FILTERING;
        desc.texture_sampler_pairs[slot] = {SG_SHADERSTAGE_FRAGMENT, static_cast<uint8_t>(slot), static_cast<uint8_t>(slot), name};
    };
    if (fragment == "pbr.frag") {
        block(1, {{"alphaPass", SG_UNIFORMTYPE_INT},
                  {"dirLight.direction", SG_UNIFORMTYPE_FLOAT3},
                  {"dirLight.diffuse", SG_UNIFORMTYPE_FLOAT3},
                  {"activePointLightCount", SG_UNIFORMTYPE_INT},
                  {"cameraPos", SG_UNIFORMTYPE_FLOAT3},
                  {"maxReflectionLOD", SG_UNIFORMTYPE_FLOAT},
                  {"ambientIntensity", SG_UNIFORMTYPE_FLOAT},
                  {"pointShadowCount", SG_UNIFORMTYPE_INT}});
        for (const auto* name : {"material.albedoMap", "material.metallicMap", "material.roughnessMap", "material.aoMap", "material.normalMap"})
            texture(name, SG_IMAGETYPE_2D);
        texture("irradianceMap", SG_IMAGETYPE_CUBE);
        texture("prefilterMap", SG_IMAGETYPE_CUBE);
        texture("brdfLUT", SG_IMAGETYPE_2D);
        texture("shadowMap", SG_IMAGETYPE_2D, true);
        texture("pointShadowMap", SG_IMAGETYPE_ARRAY, true);
        desc.views[textureSlot].storage_buffer.stage = SG_SHADERSTAGE_FRAGMENT;
        desc.views[textureSlot].storage_buffer.readonly = true;
    } else if (fragment == "alpha.frag") {
        block(1, {{"alphaPass", SG_UNIFORMTYPE_INT}});
        texture("material.diffuse", SG_IMAGETYPE_2D);
    } else if (fragment == "skybox.frag") {
        block(1, {{"exposure", SG_UNIFORMTYPE_FLOAT}});
        texture("skybox", SG_IMAGETYPE_CUBE);
    } else if (fragment == "point_shadow.frag") {
        block(1, {{"lightPos", SG_UNIFORMTYPE_FLOAT3}, {"farPlane", SG_UNIFORMTYPE_FLOAT}});
    } else if (fragment == "cubemap_bake.frag") {
        texture("equirectangularMap", SG_IMAGETYPE_2D);
    } else if (fragment == "irradiance_convolution.frag" || fragment == "prefilter.frag") {
        texture("environmentMap", SG_IMAGETYPE_CUBE);
        if (fragment == "prefilter.frag")
            block(1, {{"roughness", SG_UNIFORMTYPE_FLOAT}});
    }
}

} // namespace

void setAssetRoot(const std::string& root) {
    assetRoot = root;
    if (!assetRoot.empty() && assetRoot.back() != '/') {
        assetRoot += '/';
    }
}

const std::string& getAssetRoot() {
    return assetRoot;
}

ShaderSource::ShaderSource(std::string v, std::string f) : vertexPath(std::move(v)), fragmentPath(std::move(f)) {
    auto load = [](const std::string& path) {
        const auto entry = Shaders::Registry.find(std::filesystem::path(path).filename().string());
        if (entry != Shaders::Registry.end())
            return std::string(entry->second);
        std::ifstream file(path);
        return std::string(std::istreambuf_iterator<char>(file), {});
    };
    vertexSourceCode = load(vertexPath);
    fragmentSourceCode = load(fragmentPath);
    describeShader(*this);
}

bool ShaderSource::isValid() const {
    return !vertexSourceCode.empty() && !fragmentSourceCode.empty();
}

Shader::Shader(std::shared_ptr<ShaderSource> source, unsigned int shaderId) : id(shaderId) {
    if (!sg_isvalid() || !source || !source->isValid())
        return;
    auto desc = source->interface;
    desc.vertex_func.source = source->vertexSourceCode.c_str();
    desc.fragment_func.source = source->fragmentSourceCode.c_str();
    desc.label = source->fragmentPath.c_str();
    for (int slot = 0; slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS; ++slot) {
        const auto& block = desc.uniform_blocks[slot];
        if (block.layout == SG_UNIFORMLAYOUT_STD140) {
            std::cerr << "[Error] Shader::set requires native, tightly packed uniform metadata\n";
            return;
        }
        uniformData[slot].resize(block.size);
        size_t offset = 0;
        for (const auto& member : block.glsl_uniforms) {
            if (!member.glsl_name)
                break;
            const auto size = uniformSize(member.type) * std::max<int>(1, member.array_count);
            if (!size || offset + size > block.size) {
                std::cerr << "[Error] Invalid shader uniform metadata\n";
                return;
            }
            uniforms.emplace(member.glsl_name, Uniform{slot, offset, size, member.type});
            offset += size;
        }
    }
    for (int i = 0; i < 3; ++i) {
        sg_sampler_desc sampler{};
        sampler.min_filter = sampler.mag_filter = i == 2 ? SG_FILTER_NEAREST : SG_FILTER_LINEAR;
        sampler.mipmap_filter = i == 2 ? SG_FILTER_NEAREST : SG_FILTER_LINEAR;
        sampler.wrap_u = sampler.wrap_v = sampler.wrap_w = i == 0 ? SG_WRAP_MIRRORED_REPEAT : SG_WRAP_CLAMP_TO_EDGE;
        samplers[i] = sg_make_sampler(sampler);
    }
    for (const auto& pair : desc.texture_sampler_pairs) {
        if (!pair.glsl_name)
            continue;
        textures.emplace(pair.glsl_name, pair.view_slot);
        const bool depth = desc.samplers[pair.sampler_slot].sampler_type == SG_SAMPLERTYPE_NONFILTERING;
        const bool material = std::string(pair.glsl_name).find("material.") == 0;
        bindings.samplers[pair.sampler_slot] = samplers[depth ? 2 : material ? 0 : 1];
    }
    for (int slot = 0; slot < SG_MAX_VIEW_BINDSLOTS; ++slot)
        if (desc.views[slot].storage_buffer.stage != SG_SHADERSTAGE_NONE)
            lightBufferSlot = slot;

    layout.buffers[0].stride = sizeof(Vertex);
    const int offsets[] = {offsetof(Vertex, Position), offsetof(Vertex, TexCoords), offsetof(Vertex, Normal), offsetof(Vertex, Tangent)};
    for (int i = 0; i < 4; ++i) {
        if (desc.attrs[i].base_type == SG_SHADERATTRBASETYPE_UNDEFINED && desc.attrs[4].base_type == SG_SHADERATTRBASETYPE_UNDEFINED)
            continue;
        layout.attrs[i] = {0, offsets[i], i == 1 ? SG_VERTEXFORMAT_FLOAT2 : SG_VERTEXFORMAT_FLOAT3};
    }
    if (desc.attrs[4].base_type != SG_SHADERATTRBASETYPE_UNDEFINED) {
        layout.buffers[1].stride = sizeof(glm::mat4);
        layout.buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE;
        for (int i = 0; i < 4; ++i)
            layout.attrs[4 + i] = {1, static_cast<int>(i * sizeof(glm::vec4)), SG_VERTEXFORMAT_FLOAT4};
        const glm::mat4 identity(1.0f);
        sg_buffer_desc buffer{};
        buffer.data = {&identity, sizeof(identity)};
        identityInstance = sg_make_buffer(buffer);
    }
    shader = sg_make_shader(desc);
}

Shader::~Shader() {
    if (!sg_isvalid())
        return;
    for (auto handle : pipelines)
        sg_destroy_pipeline(handle);
    for (auto handle : samplers)
        sg_destroy_sampler(handle);
    sg_destroy_buffer(identityInstance);
    sg_destroy_shader(shader);
}

bool Shader::isValid() const {
    return sg_isvalid() && sg_query_shader_state(shader) == SG_RESOURCESTATE_VALID;
}

bool Shader::hasUniform(const std::string& name) const {
    return uniforms.count(name) != 0;
}

void Shader::setUniform(const std::string& name, const void* data, size_t size, sg_uniform_type type) const {
    const auto range = uniforms.equal_range(name);
    for (auto it = range.first; it != range.second; ++it) {
        const auto& uniform = it->second;
        if (uniform.size == size && uniform.type == type)
            std::memcpy(uniformData[uniform.block].data() + uniform.offset, data, size);
    }
}
void Shader::set(const std::string& name, bool value) const {
    set(name, static_cast<int>(value));
}
void Shader::set(const std::string& name, int value) const {
    setUniform(name, &value, sizeof(value), SG_UNIFORMTYPE_INT);
}
void Shader::set(const std::string& name, float value) const {
    setUniform(name, &value, sizeof(value), SG_UNIFORMTYPE_FLOAT);
}
void Shader::set(const std::string& name, const glm::vec2& value) const {
    setUniform(name, &value[0], 8, SG_UNIFORMTYPE_FLOAT2);
}
void Shader::set(const std::string& name, const glm::vec3& value) const {
    setUniform(name, &value[0], 12, SG_UNIFORMTYPE_FLOAT3);
}
void Shader::set(const std::string& name, const glm::mat4& value) const {
    setUniform(name, &value[0][0], 64, SG_UNIFORMTYPE_MAT4);
}

void Shader::setTexture(const std::string& name, unsigned int texture) {
    const auto it = textures.find(name);
    if (it != textures.end())
        bindings.views[it->second] = {texture};
}
void Shader::setLightBuffer(sg_view view) {
    if (lightBufferSlot >= 0)
        bindings.views[lightBufferSlot] = view;
}

sg_pipeline Shader::pipeline(ShaderPass pass) {
    auto& handle = pipelines[static_cast<size_t>(pass)];
    if (handle.id)
        return handle;
    sg_pipeline_desc desc{};
    desc.shader = shader;
    desc.layout = layout;
    desc.index_type = SG_INDEXTYPE_UINT32;
    desc.face_winding = SG_FACEWINDING_CCW;
    desc.cull_mode = SG_CULLMODE_BACK;
    desc.depth.compare = SG_COMPAREFUNC_LESS;
    desc.depth.write_enabled = true;
    if (pass == ShaderPass::Opaque || pass == ShaderPass::Translucent) {
        auto& blend = desc.colors[0].blend;
        blend.enabled = true;
        blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
        blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        desc.depth.write_enabled = pass != ShaderPass::Translucent;
    } else if (pass == ShaderPass::Skybox) {
        desc.cull_mode = SG_CULLMODE_NONE;
        desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
        desc.depth.write_enabled = false;
    } else if (pass == ShaderPass::Shadow) {
        desc.sample_count = 1;
        desc.colors[0].pixel_format = SG_PIXELFORMAT_NONE;
        desc.depth.pixel_format = SG_PIXELFORMAT_DEPTH;
    } else {
        desc.sample_count = 1;
        desc.cull_mode = SG_CULLMODE_NONE;
        desc.depth.pixel_format = SG_PIXELFORMAT_NONE;
        desc.depth.compare = SG_COMPAREFUNC_ALWAYS;
        desc.depth.write_enabled = false;
        desc.colors[0].pixel_format = pass == ShaderPass::Brdf ? SG_PIXELFORMAT_RG16F : SG_PIXELFORMAT_RGBA16F;
    }
    handle = sg_make_pipeline(desc);
    return handle;
}

void Shader::draw(const Mesh& mesh, ShaderPass pass, sg_buffer instances, int instanceCount) {
    if (!isValid() || !mesh.isReady() || instanceCount <= 0 || sg_query_pass_state() != SG_PASSSTATE_RENDER)
        return;
    const auto pipe = pipeline(pass);
    if (sg_query_pipeline_state(pipe) != SG_RESOURCESTATE_VALID)
        return;
    auto drawBindings = bindings;
    drawBindings.vertex_buffers[0] = mesh.vertexBuffer;
    drawBindings.index_buffer = mesh.indexBuffer;
    if (identityInstance.id)
        drawBindings.vertex_buffers[1] = instances.id ? instances : identityInstance;
    sg_apply_pipeline(pipe);
    sg_apply_bindings(drawBindings);
    for (int slot = 0; slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS; ++slot)
        if (!uniformData[slot].empty())
            sg_apply_uniforms(slot, {uniformData[slot].data(), uniformData[slot].size()});
    sg_draw(0, mesh.indexCount, instanceCount);
}

TextureMaterial::~TextureMaterial() {
    if (ownsTexture)
        destroyTexture(textureId);
}
void TextureMaterial::bind() {
    if (shader)
        shader->setTexture("material.diffuse", textureId);
}

PbrMaterial::~PbrMaterial() {
    if (isAlbedoAllocated)
        destroyTexture(albedoMap);
    if (isMetallicAllocated)
        destroyTexture(metallicMap);
    if (isRoughnessAllocated)
        destroyTexture(roughnessMap);
    if (isAoAllocated)
        destroyTexture(aoMap);
    if (isNormalAllocated)
        destroyTexture(normalMap);
}

void PbrMaterial::setAlbedoMap(unsigned int texture) {
    if (albedoMap == texture)
        return;
    if (isAlbedoAllocated)
        destroyTexture(albedoMap);
    isAlbedoAllocated = false;
    albedoMap = texture;
}
void PbrMaterial::setMetallicMap(unsigned int texture) {
    if (metallicMap == texture)
        return;
    if (isMetallicAllocated)
        destroyTexture(metallicMap);
    isMetallicAllocated = false;
    metallicMap = texture;
}
void PbrMaterial::setRoughnessMap(unsigned int texture) {
    if (roughnessMap == texture)
        return;
    if (isRoughnessAllocated)
        destroyTexture(roughnessMap);
    isRoughnessAllocated = false;
    roughnessMap = texture;
}
void PbrMaterial::setAoMap(unsigned int texture) {
    if (aoMap == texture)
        return;
    if (isAoAllocated)
        destroyTexture(aoMap);
    isAoAllocated = false;
    aoMap = texture;
}
void PbrMaterial::setNormalMap(unsigned int texture) {
    if (normalMap == texture)
        return;
    if (isNormalAllocated)
        destroyTexture(normalMap);
    isNormalAllocated = false;
    normalMap = texture;
}

void PbrMaterial::bind() {
    if (!shader)
        return;
    shader->setTexture("material.albedoMap", albedoMap);
    shader->setTexture("material.metallicMap", metallicMap);
    shader->setTexture("material.roughnessMap", roughnessMap);
    shader->setTexture("material.aoMap", aoMap);
    shader->setTexture("material.normalMap", normalMap);
}

ShaderSource AlphaShader::GetSource() {
    return ShaderSource(getAssetRoot() + "shaders/alpha.vert", getAssetRoot() + "shaders/alpha.frag");
}

ShaderSource PbrShader::GetSource() {
    return ShaderSource(getAssetRoot() + "shaders/alpha.vert", getAssetRoot() + "shaders/pbr.frag");
}

Model::Model(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
    if (mesh || material) {
        subMeshes.emplace_back(std::move(mesh), std::move(material));
    }
    calculateBounds();
}

Model::Model(std::vector<SubMesh> subMeshes) : subMeshes(std::move(subMeshes)) {
    calculateBounds();
}

void Model::calculateBounds() {
    bool hasVertices = false;
    glm::vec3 minPos(std::numeric_limits<float>::max());
    glm::vec3 maxPos(std::numeric_limits<float>::lowest());

    for (const auto& subMesh : subMeshes) {
        if (!subMesh.mesh || subMesh.mesh->vertices.empty())
            continue;

        for (const auto& v : subMesh.mesh->vertices) {
            minPos = glm::min(minPos, v.Position);
            maxPos = glm::max(maxPos, v.Position);
            hasVertices = true;
        }
    }

    if (!hasVertices) {
        boundsCenter = glm::vec3(0.0f);
        boundsRadius = 0.0f;
        return;
    }

    boundsCenter = (minPos + maxPos) * 0.5f;

    float maxDistSq = 0.0f;
    for (const auto& subMesh : subMeshes) {
        if (!subMesh.mesh || subMesh.mesh->vertices.empty())
            continue;

        for (const auto& v : subMesh.mesh->vertices) {
            glm::vec3 diff = v.Position - boundsCenter;
            float distSq = glm::dot(diff, diff);
            if (distSq > maxDistSq) {
                maxDistSq = distSq;
            }
        }
    }
    boundsRadius = std::sqrt(maxDistSq);
}

glm::mat4 Transform::getWorldMatrix() const {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::mat4_cast(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 toPivot = glm::translate(glm::mat4(1.0f), pivot);
    glm::mat4 fromPivot = glm::translate(glm::mat4(1.0f), -pivot);

    return T * toPivot * R * S * fromPivot;
}

bool Object::isVisible(const Frustum& frustum) const {
    return isVisible(frustum, getWorldMatrix());
}

bool Object::isVisible(const Frustum& frustum, const glm::mat4& worldMatrix) const {
    if (!model)
        return false;

    if (model->boundsRadius <= 0.0f) {
        model->calculateBounds();
    }

    glm::vec3 center = glm::vec3(worldMatrix * glm::vec4(model->boundsCenter, 1.0f));

    glm::vec3 absScale = glm::abs(scale);
    float maxScale = glm::max(glm::max(absScale.x, absScale.y), absScale.z);

    float radius = model->boundsRadius * maxScale;

    return frustum.intersectsSphere(center, radius);
}

glm::vec3 Transform::getFront() const {
    return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Transform::getRight() const {
    return glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Transform::getUp() const {
    return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
}
} // namespace knot
