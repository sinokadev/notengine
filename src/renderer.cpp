#include <knot/renderer.h>
#include <sokol/glfw_glue.h>
#include <sokol/sokol_log.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <unordered_set>

namespace knot {
static_assert(sizeof(GPUMovingPointLight) == 48, "Point-light data must match the GLSL std430 layout");
namespace {
std::shared_ptr<Shader> loadShader(const char* name, unsigned int id) {
    const auto path = getAssetRoot() + "shaders/" + name;
    return std::make_shared<Shader>(std::make_shared<ShaderSource>(path + ".vert", path + ".frag"), id);
}

unsigned int renderTexture(int size, sg_pixel_format format, int layers = 1) {
    sg_image_desc desc{};
    desc.type = layers == 1 ? SG_IMAGETYPE_2D : SG_IMAGETYPE_ARRAY;
    desc.width = desc.height = size;
    desc.num_slices = layers;
    desc.sample_count = 1;
    desc.pixel_format = format;
    desc.usage.depth_stencil_attachment = format == SG_PIXELFORMAT_DEPTH;
    desc.usage.color_attachment = !desc.usage.depth_stencil_attachment;
    return createTextureView(sg_make_image(desc));
}

sg_view attachment(unsigned int texture, int slice = 0) {
    sg_view_desc desc{};
    const auto image = sg_query_view_image({texture});
    auto& target = sg_query_image_pixelformat(image) == SG_PIXELFORMAT_DEPTH ? desc.depth_stencil_attachment : desc.color_attachment;
    target.image = image;
    target.slice = slice;
    return sg_make_view(desc);
}

// Grow buffers only when needed. Each transient buffer is written once per frame;
// callers acquire a separate buffer for every batch consumed by a draw.
bool upload(sg_buffer& buffer, const sg_range& data, bool storage = false) {
    if (!buffer.id || sg_query_buffer_size(buffer) < data.size) {
        sg_destroy_buffer(buffer);
        sg_buffer_desc desc{};
        desc.size = std::max<size_t>(256, data.size * 2);
        desc.usage.storage_buffer = storage;
        desc.usage.vertex_buffer = !storage;
        desc.usage.write_transient = true;
        buffer = sg_make_buffer(desc);
    }
    if (sg_query_buffer_state(buffer) != SG_RESOURCESTATE_VALID)
        return false;
    sg_write_buffer_desc write{};
    write.dst.buffer = buffer;
    write.src.data = data;
    sg_write_buffer_transient(write);
    return true;
}
} // namespace

Renderer& Renderer::get() {
    static Renderer renderer;
    return renderer;
}
Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init() {
    if (initialized)
        return true;
    if (!glfwGetCurrentContext() || sg_isvalid())
        return false;
    sg_desc desc{};
    desc.environment = glfw_environment();
    desc.logger.func = slog_func;
    desc.buffer_pool_size = 4096;
    desc.image_pool_size = 4096;
    desc.view_pool_size = 8192;
    desc.sampler_pool_size = 512;
    desc.pipeline_pool_size = 512;
    sg_setup(desc);
    if (!sg_isvalid())
        return false;
    initialized = true;

    skyboxMesh = createCube();
    skyboxShader = loadShader("skybox", 999999);
    dirShadowShader = loadShader("dir_shadow", 999997);
    pointShadowShader = loadShader("point_shadow", 999996);
    generateBRDFLUT();
    depthMap = renderTexture(SHADOW_RESOLUTION, SG_PIXELFORMAT_DEPTH);
    if (depthMap)
        depthAttachment = attachment(depthMap);
    pointDepthMap = renderTexture(POINT_SHADOW_RESOLUTION, SG_PIXELFORMAT_DEPTH, 6);
    if (pointDepthMap)
        for (int face = 0; face < 6; ++face)
            pointDepthAttachments.push_back(attachment(pointDepthMap, face));

    const std::array<unsigned char, 24> black{};
    sg_image_desc image{};
    image.type = SG_IMAGETYPE_CUBE;
    image.width = image.height = 1;
    image.data.mip_levels[0] = {black.data(), black.size()};
    fallbackCubemap = createTextureView(sg_make_image(image));
    processPointLights({});
    if (!skyboxMesh->isReady() || !skyboxShader->isValid() || !dirShadowShader->isValid() || !pointShadowShader->isValid() || !brdfLUTTexture ||
        !depthMap || !pointDepthMap || !fallbackCubemap || sg_query_view_state(lightView) != SG_RESOURCESTATE_VALID) {
        shutdown();
        return false;
    }
    return true;
}

void Renderer::shutdown() {
    if (!initialized)
        return;
    if (mainPassActive)
        sg_end_pass();
    mainPassActive = false;
    skyboxShader.reset();
    dirShadowShader.reset();
    pointShadowShader.reset();
    skyboxMesh.reset();
    for (const auto& entry : lightBuffers) {
        sg_destroy_view(entry.view);
        sg_destroy_buffer(entry.buffer);
    }
    lightBuffers.clear();
    for (auto buffer : instanceBuffers)
        sg_destroy_buffer(buffer);
    instanceBuffers.clear();
    nextLightBuffer = nextInstanceBuffer = 0;
    sg_destroy_view(depthAttachment);
    for (auto view : pointDepthAttachments)
        sg_destroy_view(view);
    pointDepthAttachments.clear();
    for (auto texture : {brdfLUTTexture, depthMap, pointDepthMap, fallbackCubemap})
        destroyTexture(texture);
    lightView = {};
    instanceBuffer = {};
    depthAttachment = {};
    brdfLUTTexture = depthMap = pointDepthMap = fallbackCubemap = 0;
    pointShadowCount = 0;
    shadowedPointLights.clear();
    sg_shutdown();
    initialized = false;
    framebufferWidth = framebufferHeight = 0;
}

void Renderer::generateBRDFLUT() {
    auto shader = loadShader("brdf", 999998);
    if (!shader->isValid())
        return;
    brdfLUTTexture = renderTexture(512, SG_PIXELFORMAT_RG16F);
    if (!brdfLUTTexture)
        return;
    Mesh quad;
    quad.vertices = {{{-1, -1, 0}, {0, 0}, {}, {}}, {{1, -1, 0}, {1, 0}, {}, {}}, {{1, 1, 0}, {1, 1}, {}, {}}, {{-1, 1, 0}, {0, 1}, {}, {}}};
    quad.indices = {0, 1, 2, 0, 2, 3};
    quad.setup();
    const auto target = attachment(brdfLUTTexture);
    sg_pass pass{};
    pass.attachments.colors[0] = target;
    sg_begin_pass(pass);
    shader->draw(quad, ShaderPass::Brdf);
    sg_end_pass();
    sg_destroy_view(target);
}

void Renderer::beginFrame(int width, int height, glm::vec4 color) {
    if (!initialized)
        return;
    if (mainPassActive)
        sg_end_pass();
    mainPassActive = false;
    framebufferWidth = width;
    framebufferHeight = height;
    clearColor = color;
    clearPending = true;
}

bool Renderer::beginMainPass() {
    if (!initialized || framebufferWidth <= 0 || framebufferHeight <= 0)
        return false;
    if (mainPassActive)
        return true;
    sg_pass pass{};
    pass.swapchain = glfw_swapchain();
    pass.action.colors[0].load_action = clearPending ? SG_LOADACTION_CLEAR : SG_LOADACTION_LOAD;
    pass.action.colors[0].clear_value = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
    pass.action.depth.load_action = clearPending ? SG_LOADACTION_CLEAR : SG_LOADACTION_LOAD;
    pass.action.depth.store_action = SG_STOREACTION_STORE;
    pass.action.depth.clear_value = 1.0f;
    sg_begin_pass(pass);
    sg_apply_viewport(0, 0, framebufferWidth, framebufferHeight, false);
    mainPassActive = true;
    clearPending = false;
    return true;
}

void Renderer::endFrame() {
    if (!initialized)
        return;
    if (beginMainPass())
        sg_end_pass();
    mainPassActive = false;
    sg_commit();
    nextInstanceBuffer = nextLightBuffer = 0;
}

void Renderer::processDirLights(const std::shared_ptr<Shader>& shader, const std::vector<const DirLight*>& dirLights) {
    if (!shader)
        return;

    if (!dirLights.empty()) {
        const auto* dirLight = dirLights.front();

        shader->set("dirLight.direction", dirLight->getDirection());
        shader->set("dirLight.ambient", dirLight->ambient);
        shader->set("dirLight.diffuse", dirLight->diffuse);
        shader->set("dirLight.specular", dirLight->specular);
    } else {
        shader->set("dirLight.direction", glm::vec3(0.0f, -1.0f, 0.0f));

        shader->set("dirLight.ambient", glm::vec3(0.0f));
        shader->set("dirLight.diffuse", glm::vec3(0.0f));
        shader->set("dirLight.specular", glm::vec3(0.0f));
    }
}

void Renderer::processPointLights(const std::vector<const PbrPointLight*>& pointLights) {
    std::vector<GPUMovingPointLight> gpuLights;
    gpuLights.reserve(pointLights.size());

    std::unordered_map<const PbrPointLight*, int> shadowLayers;
    for (std::size_t i = 0; i < shadowedPointLights.size(); ++i) {
        shadowLayers.emplace(shadowedPointLights[i], static_cast<int>(i));
    }

    for (const auto* light : pointLights) {
        GPUMovingPointLight gpuLight;

        const auto layer = shadowLayers.find(light);
        const int shadowLayer = light->castsShadow && layer != shadowLayers.end() ? layer->second : -1;
        gpuLight.position = glm::vec4(light->position, static_cast<float>(shadowLayer));

        gpuLight.color = glm::vec4(light->color, light->intensity);

        gpuLight.radius = 5.0f * std::sqrt(std::max(0.0f, light->intensity));

        gpuLight.constant = 1.0f;
        gpuLight.linear = 0.09f;
        gpuLight.quadratic = 0.032f;

        gpuLights.push_back(gpuLight);
    }

    if (gpuLights.empty())
        gpuLights.push_back({});
    if (nextLightBuffer == lightBuffers.size())
        lightBuffers.push_back({});
    auto& entry = lightBuffers[nextLightBuffer++];
    const size_t size = gpuLights.size() * sizeof(GPUMovingPointLight);
    if (entry.buffer.id && sg_query_buffer_size(entry.buffer) < size) {
        sg_destroy_view(entry.view);
        entry.view = {};
    }
    if (!upload(entry.buffer, {gpuLights.data(), size}, true))
        return;
    if (!entry.view.id) {
        sg_view_desc desc{};
        desc.storage_buffer.buffer = entry.buffer;
        entry.view = sg_make_view(desc);
    }
    lightView = entry.view;
}

void Renderer::renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera,
                               float aspectRatio) {
    if (!model || model->subMeshes.empty() || instances.empty())
        return;

    if (!beginMainPass())
        return;
    uploadInstances(instances);

    for (const auto& subMesh : model->subMeshes) {
        if (!subMesh.mesh || !subMesh.material || !subMesh.mesh->isReady())
            continue;

        auto shader = subMesh.material->getShader();

        if (!shader || !shader->isValid())
            continue;

        subMesh.material->bind();
        shader->set("isInstanced", true);

        shader->set("view", camera.getViewMatrix());
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);

        shader->draw(*subMesh.mesh, ShaderPass::Opaque, instanceBuffer, static_cast<int>(instances.size()));
    }
}

void Renderer::renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& worldMatrix, const Camera& camera, float aspectRatio) {
    if (!beginMainPass() || !model || model->subMeshes.empty())
        return;

    for (const auto& subMesh : model->subMeshes) {
        if (!subMesh.mesh || !subMesh.material || !subMesh.mesh->isReady())
            continue;

        auto shader = subMesh.material->getShader();
        if (!shader || !shader->isValid())
            continue;

        subMesh.material->bind();

        shader->set("isInstanced", false);

        shader->set("model", worldMatrix);
        shader->set("view", camera.getViewMatrix());
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);

        shader->draw(*subMesh.mesh, ShaderPass::Opaque);
    }
}

bool Renderer::renderObject(const VisibleInstance& instance, const Camera& camera, float aspectRatio) {
    if (!initialized || !instance.object)
        return false;

    const auto& object = *instance.object;

    if (!object.model)
        return true;

    if (object.model->subMeshes.empty())
        return false;

    renderSingle(object.model, instance.worldMatrix, camera, aspectRatio);
    return true;
}

bool Renderer::renderObject(const Object& object, const Camera& camera, float aspectRatio) {
    return renderObject(VisibleInstance{&object, object.getWorldMatrix()}, camera, aspectRatio);
}

void Renderer::renderSkybox(unsigned int cubemapID, const Camera& camera, float aspectRatio) {
    if (!cubemapID || !beginMainPass())
        return;
    skyboxShader->set("view", glm::mat4(glm::mat3(camera.getViewMatrix())));
    skyboxShader->set("projection", camera.getProjectionMatrix(aspectRatio));
    skyboxShader->set("exposure", AMBIENT_INTENSITY);
    skyboxShader->setTexture("skybox", cubemapID);
    skyboxShader->draw(*skyboxMesh, ShaderPass::Skybox);
}

void Renderer::renderObjects(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, float aspectRatio) {
    const auto& camera = scene.getCamera();
    const auto dirLights = scene.getLightManager().getDirLights();
    const auto pointLights = scene.getLightManager().getPointLights();
    auto setupSceneUniforms = [&](const std::shared_ptr<Shader>& shader) {
        if (!shader || !shader->isValid())
            return;


        processDirLights(shader, dirLights);

        shader->set("activePointLightCount", static_cast<int>(pointLights.size()));

        shader->setTexture("irradianceMap", scene.getIrradianceMap() ? scene.getIrradianceMap() : fallbackCubemap);
        shader->setTexture("prefilterMap", scene.getPrefilterMap() ? scene.getPrefilterMap() : fallbackCubemap);
        shader->setTexture("brdfLUT", brdfLUTTexture);
        shader->setTexture("shadowMap", depthMap);
        shader->setTexture("pointShadowMap", pointDepthMap);
        shader->setLightBuffer(lightView);
        shader->set("pointShadowCount", pointShadowCount);
        shader->set("maxReflectionLOD", scene.getPrefilterMap() ? 4.0f : 0.0f);

        shader->set("ambientIntensity", AMBIENT_INTENSITY);

        shader->set("lightSpaceMatrix", lightSpaceMatrix);
    };

    struct TranslucentDraw {
        const SubMesh* subMesh;
        glm::mat4 worldMatrix;
        float depth;
    };
    std::vector<TranslucentDraw> translucentDraws;
    std::unordered_set<unsigned int> preparedShaders;
    const auto view = camera.getViewMatrix();

    // Establish the opaque depth buffer before blending any translucent pixels.

    for (const auto& [modelKey, instances] : instanceGroups) {
        if (instances.empty())
            continue;

        const auto& model = instances.front().object->model;
        if (!model || model->subMeshes.empty())
            continue;

        for (const auto& subMesh : model->subMeshes) {
            if (!subMesh.material)
                continue;

            auto shader = subMesh.material->getShader();
            if (!shader || !shader->isValid())
                continue;

            if (preparedShaders.insert(shader->getShaderProgram()).second) {
                setupSceneUniforms(shader);
                shader->set("alphaPass", 1);
            }
            // Custom shaders without the pass uniform are drawn only once.
            if (subMesh.mesh && subMesh.mesh->isReady() && shader->hasUniform("alphaPass")) {
                for (const auto& inst : instances) {
                    const auto center = view * inst.worldMatrix * glm::vec4(subMesh.mesh->boundsCenter, 1.0f);
                    translucentDraws.push_back({&subMesh, inst.worldMatrix, -center.z});
                }
            }
        }

        if (instances.size() >= INSTANCE_THRESHOLD) { // instanced objects render
            renderInstanced(model, instances, camera, aspectRatio);
        } else { // just render
            for (const auto& inst : instances) {
                renderObject(inst, camera, aspectRatio);
            }
        }
    }

    // Sort globally across models and instances. Depth testing remains enabled,
    // but blended fragments must not prevent farther surfaces from contributing.
    std::stable_sort(translucentDraws.begin(), translucentDraws.end(), [](const auto& a, const auto& b) { return a.depth > b.depth; });
    for (const auto& draw : translucentDraws) {
        const auto& subMesh = *draw.subMesh;
        auto shader = subMesh.material->getShader();
        subMesh.material->bind();
        shader->set("alphaPass", 2);
        shader->set("isInstanced", false);
        shader->set("model", draw.worldMatrix);
        shader->set("view", view);
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);
        shader->draw(*subMesh.mesh, ShaderPass::Translucent);
        // Preserve the default behavior of subsequent direct renderSingle calls.
        shader->set("alphaPass", 0);
    }
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized || framebufferWidth <= 0 || framebufferHeight <= 0 || aspectRatio <= 0.0f)
        return false;

    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();
    auto& lightManager = scene.getLightManager();

    const auto pointLights = lightManager.getPointLights();

    std::unordered_map<const Model*, std::vector<VisibleInstance>> instanceGroups;

    std::unordered_map<const Model*, std::vector<VisibleInstance>> shadowGroups;
    const Frustum& frustum = camera.getFrustum(aspectRatio);

    // Extract instanced objects only
    for (const auto& object : objectManager.getObjects()) {
        if (!object->model || object->model->subMeshes.empty())
            continue;

        glm::mat4 worldMatrix = object->getWorldMatrix();

        shadowGroups[object->model.get()].push_back(VisibleInstance{object.get(), worldMatrix});

        if (!object->isVisible(frustum, worldMatrix))
            continue;

        instanceGroups[object->model.get()].push_back(VisibleInstance{object.get(), worldMatrix});
    }

    if (mainPassActive) {
        sg_end_pass();
        mainPassActive = false;
    }
    renderShadow(scene, shadowGroups);
    // Upload the layer mapping selected for this frame, including unshadowed lights.
    processPointLights(pointLights);

    if (!beginMainPass())
        return false;
    renderSkybox(scene.getCubeMap(), camera, aspectRatio);

    renderObjects(scene, instanceGroups, aspectRatio);

    return true;
}

void Renderer::renderDirShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    if (!dirShadowShader || !dirShadowShader->isValid())
        return;

    const auto dirLights = scene.getLightManager().getDirLights();
    glm::vec3 lightDir(0.0f, -1.0f, 0.0f);

    if (!dirLights.empty()) {
        lightDir = dirLights.front()->getDirection();
    }

    // light normalize
    if (glm::length(lightDir) < 0.0001f) { // is 0 vector?
        lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    } else {
        lightDir = glm::normalize(lightDir);
    }

    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 0.1f, 30.0f);

    glm::vec3 lightPos = -lightDir * 10.0f;
    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDir, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 lightView = glm::lookAt(lightPos, target, up);

    lightSpaceMatrix = lightProjection * lightView;

    sg_pass pass{};
    pass.attachments.depth_stencil = depthAttachment;
    pass.action.depth.store_action = SG_STOREACTION_STORE;
    sg_begin_pass(pass);
    dirShadowShader->set("lightSpaceMatrix", lightSpaceMatrix);
    renderShadowObjects(instanceGroups, dirShadowShader);
    sg_end_pass();
}

void Renderer::uploadInstances(const std::vector<VisibleInstance>& instances) {
    std::vector<InstanceData> data;
    data.reserve(instances.size());
    for (const auto& instance : instances)
        data.push_back({instance.worldMatrix});
    if (nextInstanceBuffer == instanceBuffers.size())
        instanceBuffers.push_back({});
    auto& buffer = instanceBuffers[nextInstanceBuffer++];
    upload(buffer, {data.data(), data.size() * sizeof(InstanceData)});
    instanceBuffer = buffer;
}

void Renderer::renderShadowObjects(const std::unordered_map<const Model*, std::vector<VisibleInstance>>& groups,
                                   const std::shared_ptr<Shader>& shader) {
    for (const auto& [model, instances] : groups) {
        if (!model || instances.empty())
            continue;
        const bool instanced = instances.size() >= INSTANCE_THRESHOLD;
        shader->set("isInstanced", instanced);
        if (instanced)
            uploadInstances(instances);
        for (const auto& subMesh : model->subMeshes) {
            if (!subMesh.mesh || !subMesh.mesh->isReady())
                continue;
            if (instanced) {
                shader->draw(*subMesh.mesh, ShaderPass::Shadow, instanceBuffer, static_cast<int>(instances.size()));
            } else {
                for (const auto& instance : instances) {
                    shader->set("model", instance.worldMatrix);
                    shader->draw(*subMesh.mesh, ShaderPass::Shadow);
                }
            }
        }
    }
}

void Renderer::renderShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    renderDirShadow(scene, instanceGroups);
    renderPointShadow(scene, instanceGroups);
}

void Renderer::renderPointShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& groups) {
    shadowedPointLights.clear();
    const auto limit = sg_query_limits().max_image_array_layers / 6;
    for (auto* light : scene.getLightManager().getPointLights()) {
        if (light->castsShadow && shadowedPointLights.size() < static_cast<size_t>(limit))
            shadowedPointLights.push_back(light);
    }
    pointShadowCount = static_cast<int>(shadowedPointLights.size());
    const int layers = std::max(1, pointShadowCount) * 6;
    if (static_cast<int>(pointDepthAttachments.size()) != layers) {
        for (auto view : pointDepthAttachments)
            sg_destroy_view(view);
        pointDepthAttachments.clear();
        destroyTexture(pointDepthMap);
        pointDepthMap = renderTexture(POINT_SHADOW_RESOLUTION, SG_PIXELFORMAT_DEPTH, layers);
        if (!pointDepthMap) {
            pointShadowCount = 0;
            return;
        }
        for (int slice = 0; slice < layers; ++slice)
            pointDepthAttachments.push_back(attachment(pointDepthMap, slice));
    }
    const glm::vec3 directions[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 up[] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
    for (int light = 0; light < pointShadowCount; ++light) {
        const auto* source = shadowedPointLights[light];
        const auto position = source->position;
        const float farPlane = std::max(kNearPlane * 2, 5 * std::sqrt(std::max(0.0f, source->intensity)));
        const auto projection = glm::perspective(glm::radians(90.0f), 1.0f, kNearPlane, farPlane);
        std::unordered_map<const Model*, std::vector<VisibleInstance>> culled;
        for (const auto& [model, instances] : groups) {
            for (const auto& instance : instances) {
                const auto center = glm::vec3(instance.worldMatrix * glm::vec4(model->boundsCenter, 1));
                const float scale = std::max({glm::length(glm::vec3(instance.worldMatrix[0])), glm::length(glm::vec3(instance.worldMatrix[1])),
                                              glm::length(glm::vec3(instance.worldMatrix[2]))});
                if (glm::distance(position, center) <= farPlane + model->boundsRadius * scale)
                    culled[model].push_back(instance);
            }
        }
        pointShadowShader->set("lightPos", position);
        pointShadowShader->set("farPlane", farPlane);
        for (int face = 0; face < 6; ++face) {
            sg_pass pass{};
            pass.attachments.depth_stencil = pointDepthAttachments[light * 6 + face];
            pass.action.depth.store_action = SG_STOREACTION_STORE;
            sg_begin_pass(pass);
            pointShadowShader->set("lightSpaceMatrix", projection * glm::lookAt(position, position + directions[face], up[face]));
            renderShadowObjects(culled, pointShadowShader);
            sg_end_pass();
        }
    }
}
} // namespace knot
