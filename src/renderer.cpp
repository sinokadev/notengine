#include <knot/renderer.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <unordered_set>
#include <knot/mesh.h>

#define DISABLE_SKYMAP false
#define DISABLE_SHADOW false

namespace knot {

Renderer& Renderer::get() {
    static Renderer instance;
    return instance;
}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(GLADloadfunc loadProc) {
    std::cout << "[Info] Not Engine Renderer Init" << std::endl;

    // Load GL
    if (!gladLoadGL(loadProc)) {
        std::cerr << "[Error] Failed to load OpenGL functions" << std::endl;
        return false;
    }

    // GL Config
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glGenBuffers(1, &lightSSBO);
    glGenBuffers(1, &instanceVBO);

    // Sky Map and IBL
    skyboxMesh = createCube();
    auto skyboxSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/skybox.vert", getAssetRoot() + "shaders/skybox.frag");
    skyboxShader = std::make_shared<Shader>(skyboxSource, SKYBOX_SHADER_ID);

    generateBRDFLUT();

    // Shadow Map
    glGenFramebuffers(1, &depthMapFBO);

    // depth map texture gen
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_RESOLUTION, SHADOW_RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // depth map bind
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Error] Shadow framebuffer is not complete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Shadow Shader
    auto dirShadowSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/dir_shadow.vert", getAssetRoot() + "shaders/dir_shadow.frag");

    dirShadowShader = std::make_shared<Shader>(dirShadowSource, SHADOW_SHADER_ID);

    auto pointShadowSource =
        std::make_shared<ShaderSource>(getAssetRoot() + "shaders/point_shadow.vert", getAssetRoot() + "shaders/point_shadow.frag");

    pointShadowShader = std::make_shared<Shader>(pointShadowSource, SHADOW_SHADER_ID - 1);

    glGenFramebuffers(1, &pointDepthFBO);
    glGenTextures(1, &pointDepthMap);

    initialized = true;
    return true;
}

void Renderer::shutdown() {
    if (!initialized) {
        return;
    }

    const bool hasContext = (glfwGetCurrentContext() != nullptr);
    if (lightSSBO != 0) {
        if (hasContext) {
            glDeleteBuffers(1, &lightSSBO);
        }
        lightSSBO = 0;
    }

    if (instanceVBO != 0) {
        if (hasContext) {
            glDeleteBuffers(1, &instanceVBO);
        }
        instanceVBO = 0;
    }

    if (brdfLUTTexture != 0) {
        if (hasContext) {
            glDeleteTextures(1, &brdfLUTTexture);
        }
        brdfLUTTexture = 0;
    }

    if (quadVAO != 0) {
        if (hasContext) {
            glDeleteVertexArrays(1, &quadVAO);
            glDeleteBuffers(1, &quadVBO);
        }
        quadVAO = 0;
        quadVBO = 0;
    }

    if (hasContext) {
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteFramebuffers(1, &pointDepthFBO);
        glDeleteTextures(1, &depthMap);
        glDeleteTextures(1, &pointDepthMap);
    }
    depthMapFBO = depthMap = pointDepthFBO = pointDepthMap = 0;
    pointShadowCount = 0;
    dirShadowShader.reset();
    pointShadowShader.reset();

    skyboxMesh.reset();
    skyboxShader.reset();

    initialized = false;
}

void Renderer::renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions   // texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Renderer::generateBRDFLUT() {
    // gen texture
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // attach texture to framebuffer
    GLuint captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    // get shader
    auto brdfSource = std::make_shared<ShaderSource>(getAssetRoot() + "shaders/brdf.vert", getAssetRoot() + "shaders/brdf.frag");
    std::shared_ptr<Shader> brdfShader = std::make_shared<Shader>(brdfSource, BRDF_SHADER_ID);

    // viewport backup
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // render
    glViewport(0, 0, 512, 512);
    brdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT);
    renderQuad();

    // clean
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
}

void Renderer::beginFrame(int fbWidth, int fbHeight) {
    if (fbWidth <= 0 || fbHeight <= 0)
        return;

    framebufferWidth = fbWidth;
    framebufferHeight = fbHeight;

    glViewport(0, 0, framebufferWidth, framebufferHeight);
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

    for (const auto* light : pointLights) {
        GPUMovingPointLight gpuLight;

        gpuLight.position = glm::vec4(light->position, 1.0f);

        gpuLight.color = glm::vec4(light->color, light->intensity);

        gpuLight.radius = 5.0f * std::sqrt(light->intensity);

        gpuLight.constant = 1.0f;
        gpuLight.linear = 0.09f;
        gpuLight.quadratic = 0.032f;

        gpuLights.push_back(gpuLight);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);

    if (!gpuLights.empty()) {
        glBufferData(GL_SHADER_STORAGE_BUFFER, gpuLights.size() * sizeof(GPUMovingPointLight), gpuLights.data(), GL_DYNAMIC_DRAW);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind buffer
}

void Renderer::renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera,
                               float aspectRatio) {
    if (!model || model->subMeshes.empty() || instances.empty())
        return;

    std::vector<InstanceData> instanceData;
    instanceData.reserve(instances.size());

    for (const auto& inst : instances) {
        instanceData.push_back({inst.worldMatrix});
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_STREAM_DRAW);

    for (const auto& subMesh : model->subMeshes) {
        if (!subMesh.mesh || !subMesh.material || !subMesh.mesh->isReady())
            continue;

        auto shader = subMesh.material->getShader();

        if (!shader || !shader->isValid())
            continue;

        shader->use();
        shader->set("isInstanced", true);

        shader->set("view", camera.getViewMatrix());
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);

        subMesh.mesh->setupInstanceAttributes(instanceVBO);

        glBindVertexArray(subMesh.mesh->vao);

        glDrawElementsInstanced(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(instanceData.size()));

        glBindVertexArray(0);
    }
}

void Renderer::renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& worldMatrix, const Camera& camera, float aspectRatio) {
    if (!model || model->subMeshes.empty())
        return;

    for (const auto& subMesh : model->subMeshes) {
        if (!subMesh.mesh || !subMesh.material || !subMesh.mesh->isReady())
            continue;

        auto shader = subMesh.material->getShader();
        if (!shader || !shader->isValid())
            continue;

        shader->use();
        subMesh.material->bind();

        shader->set("isInstanced", false);

        shader->set("model", worldMatrix);
        shader->set("view", camera.getViewMatrix());
        shader->set("projection", camera.getProjectionMatrix(aspectRatio));
        shader->set("cameraPos", camera.position);

        glBindVertexArray(subMesh.mesh->vao);

        for (GLuint i = 0; i < 4; ++i) {
            glDisableVertexAttribArray(4 + i);
        }

        glDrawElements(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
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
    if (DISABLE_SKYMAP)
        return;

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    skyboxShader->use();

    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

    skyboxShader->set("view", view);
    skyboxShader->set("projection", projection);
    skyboxShader->set("exposure", AMBIENT_INTENSITY);

    glBindVertexArray(skyboxMesh->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
    skyboxShader->set("skybox", 0);

    glDrawElements(GL_TRIANGLES, skyboxMesh->indexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void Renderer::renderObjects(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups, float aspectRatio) {
    const auto& camera = scene.getCamera();
    const auto dirLights = scene.getLightManager().getDirLights();
    const auto pointLights = scene.getLightManager().getPointLights();
    auto setupSceneUniforms = [&](const std::shared_ptr<Shader>& shader) {
        if (!shader || !shader->isValid())
            return;

        shader->use();

        processDirLights(shader, dirLights);

        shader->set("activePointLightCount", static_cast<int>(pointLights.size()));

        // Irradiance Map
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene.getIrradianceMap());
        shader->set("irradianceMap", 8);

        // Prefilter Map
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene.getPrefilterMap());
        shader->set("prefilterMap", 9);

        // BRDF LUT
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        shader->set("brdfLUT", 10);

        // Shadow map
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shader->set("shadowMap", 11);

        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointDepthMap);
        shader->set("pointShadowMap", 12);
        shader->set("pointShadowCount", pointShadowCount);

        shader->set("aaxReflectionLOD", 4.0f);

        shader->set("ambientIntensity", AMBIENT_INTENSITY);

        shader->set("lightSpaceMatrix", lightSpaceMatrix);
    };

    for (const auto& [modelKey, instances] : instanceGroups) {
        // validation
        if (instances.empty())
            continue;

        const auto& model = instances.front().object->model;
        if (!model || model->subMeshes.empty())
            continue;

        // get submesh shaders
        std::unordered_set<unsigned int> preparedShaders;
        for (const auto& subMesh : model->subMeshes) {
            if (!subMesh.material)
                continue;

            auto shader = subMesh.material->getShader();
            if (!shader || !shader->isValid())
                continue;

            if (preparedShaders.insert(shader->getShaderProgram()).second) {
                setupSceneUniforms(shader);
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
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized)
        return false;

    // var
    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();
    auto& lightManager = scene.getLightManager();

    const auto dirLights = lightManager.getDirLights();
    const auto pointLights = lightManager.getPointLights();

    // ssbo upload point lights
    processPointLights(pointLights);

    // render
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

    // render
    renderShadow(scene, shadowGroups);

    renderSkybox(scene.getCubeMap(), camera, aspectRatio);

    renderObjects(scene, instanceGroups, aspectRatio);

    return true;
}

void Renderer::renderDirShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    if (DISABLE_SHADOW)
        return;
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

    // just ignore this part
    // NOTE: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 0.1f, 30.0f);

    glm::vec3 lightPos = -lightDir * 10.0f;
    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDir, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 lightView = glm::lookAt(lightPos, target, up);

    lightSpaceMatrix = lightProjection * lightView;

    // shadow map render
    glViewport(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Shadow shader
    dirShadowShader->use();

    dirShadowShader->set("lightSpaceMatrix", lightSpaceMatrix);

    // rendering
    renderShadowObjects(instanceGroups, dirShadowShader);

    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void Renderer::renderShadowObjects(const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups,
                                   const std::shared_ptr<Shader>& shader) {
    for (const auto& [model, instances] : instanceGroups) {
        if (!model || model->subMeshes.empty() || instances.empty())
            continue;

        const bool useInstancing = instances.size() >= INSTANCE_THRESHOLD;

        if (useInstancing) {
            std::vector<InstanceData> instanceData;
            instanceData.reserve(instances.size());
            for (const auto& inst : instances) {
                instanceData.push_back({inst.worldMatrix});
            }

            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_STREAM_DRAW);

            shader->set("isInstanced", true);

            for (const auto& subMesh : model->subMeshes) {
                if (!subMesh.mesh || !subMesh.mesh->isReady())
                    continue;

                subMesh.mesh->setupInstanceAttributes(instanceVBO);
                glBindVertexArray(subMesh.mesh->vao);
                glDrawElementsInstanced(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr,
                                        static_cast<GLsizei>(instanceData.size()));
            }
        } else {
            shader->set("isInstanced", false);

            for (const auto& instance : instances) {
                shader->set("model", instance.worldMatrix);
                for (const auto& subMesh : model->subMeshes) {
                    if (!subMesh.mesh || !subMesh.mesh->isReady())
                        continue;

                    glBindVertexArray(subMesh.mesh->vao);
                    // 인스턴스 attribute가 켜져 있을 수 있으니 꺼줌 (renderSingle과 동일)
                    for (GLuint i = 0; i < 4; ++i) {
                        glDisableVertexAttribArray(4 + i);
                    }
                    glDrawElements(GL_TRIANGLES, subMesh.mesh->indexCount, GL_UNSIGNED_INT, nullptr);
                }
            }
        }
    }
    glBindVertexArray(0);
}

void Renderer::renderShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    renderDirShadow(scene, instanceGroups);
    renderPointShadow(scene, instanceGroups);
}

void Renderer::renderPointShadow(Scene& scene, const std::unordered_map<const Model*, std::vector<VisibleInstance>>& instanceGroups) {
    if (DISABLE_SHADOW || !pointShadowShader || !pointShadowShader->isValid()) {
        pointShadowCount = 0;
        return;
    }

    const auto lights = scene.getLightManager().getPointLights();
    GLint maxLayers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
    const int count = static_cast<int>(std::min(lights.size(), static_cast<std::size_t>(maxLayers / 6)));

    constexpr int POINT_SHADOW_RES = 1024; 

    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointDepthMap);
    if (count != pointShadowCount && count > 0) {
        glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT24, POINT_SHADOW_RES, POINT_SHADOW_RES, count * 6, 0, GL_DEPTH_COMPONENT,
                     GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    pointShadowCount = count;
    if (count == 0)
        return;

    const glm::vec3 directions[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 up[] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};

    glViewport(0, 0, POINT_SHADOW_RES, POINT_SHADOW_RES);
    glBindFramebuffer(GL_FRAMEBUFFER, pointDepthFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    pointShadowShader->use();

    for (int light = 0; light < count; ++light) {
        const auto& position = lights[light]->position;
        const float farPlane = std::max(kNearPlane * 2.0f, 5.0f * std::sqrt(std::max(0.0f, lights[light]->intensity)));
        const auto projection = glm::perspective(glm::radians(90.0f), 1.0f, kNearPlane, farPlane);

        std::unordered_map<const Model*, std::vector<VisibleInstance>> culledShadowGroups;

        for (const auto& [model, instances] : instanceGroups) {
            for (const auto& inst : instances) {
                glm::vec3 objPos = glm::vec3(inst.worldMatrix[3]);

                float dist = glm::distance(position, objPos);
                if (dist <= farPlane + 2.0f) {
                    culledShadowGroups[model].push_back(inst);
                }
            }
        }

        if (culledShadowGroups.empty()) {
            for (int face = 0; face < 6; ++face) {
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMap, 0, light * 6 + face);
                glClear(GL_DEPTH_BUFFER_BIT);
            }
            continue;
        }

        pointShadowShader->set("lightPos", position);
        pointShadowShader->set("farPlane", farPlane);

        for (int face = 0; face < 6; ++face) {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMap, 0, light * 6 + face);
            glClear(GL_DEPTH_BUFFER_BIT);
            pointShadowShader->set("lightSpaceMatrix", projection * glm::lookAt(position, position + directions[face], up[face]));

            renderShadowObjects(culledShadowGroups, pointShadowShader);
        }
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}
} // namespace knot
