#include <knot/renderer.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cassert>
#include <knot/mesh.h>

#define AMBIENT_INTENSITY 10.0f

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

    if (!gladLoadGL(loadProc)) {
        std::cerr << "[Error] Failed to load OpenGL functions" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glGenBuffers(1, &lightSSBO);
    glGenBuffers(1, &instanceVBO);

    skyboxMesh = createCube();
    auto skyboxSource = std::make_shared<ShaderSource>(getAssetRoot() + "assets/shaders/skybox.vert", getAssetRoot() + "assets/shaders/skybox.frag");
    skyboxShader = std::make_shared<Shader>(skyboxSource, SKYBOX_SHADER_ID);

    generateBRDFLUT();

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
    // 1. 2D RG16F 텍스처 생성
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 2. 임시 Framebuffer 생성 후 텍스처 바인딩
    GLuint captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    // 3. 셰이더 로드 및 렌더링
    auto brdfSource = std::make_shared<ShaderSource>(getAssetRoot() + "assets/shaders/brdf.vert", getAssetRoot() + "assets/shaders/brdf.frag");
    std::shared_ptr<Shader> brdfShader = std::make_shared<Shader>(brdfSource, BRDF_SHADER_ID);

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport); // 기존 뷰포트 백업

    glViewport(0, 0, 512, 512);
    brdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT);
    renderQuad();

    // 4. 복구 및 자원 정리
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
}

void Renderer::beginFrame(int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth <= 0 || framebufferHeight <= 0)
        return;
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

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::renderInstanced(const std::shared_ptr<Model>& model, const std::vector<VisibleInstance>& instances, const Camera& camera,
                               float aspectRatio) {
    if (!model || !model->mesh || !model->material)
        return;

    if (!model->mesh->isReady() || instances.empty())
        return;

    auto shader = model->material->getShader();

    if (!shader || !shader->isValid())
        return;

    std::vector<InstanceData> instanceData;
    instanceData.reserve(instances.size());

    for (const auto& inst : instances) {
        instanceData.push_back({inst.worldMatrix});
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_STREAM_DRAW);

    model->material->bind();

    shader->use();
    shader->set("u_IsInstanced", true);

    shader->set("view", camera.getViewMatrix());
    shader->set("projection", camera.getProjectionMatrix(aspectRatio));
    shader->set("u_CameraPos", camera.position);

    model->mesh->setupInstanceAttributes(instanceVBO);

    glBindVertexArray(model->mesh->vao);

    glDrawElementsInstanced(GL_TRIANGLES, model->mesh->indexCount, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(instanceData.size()));

    glBindVertexArray(0);
}

void Renderer::renderSingle(const std::shared_ptr<Model>& model, const glm::mat4& worldMatrix, const Camera& camera, float aspectRatio) {
    if (!model || !model->mesh || !model->material)
        return;

    if (!model->mesh->isReady())
        return;

    auto shader = model->material->getShader();
    if (!shader || !shader->isValid())
        return;

    shader->use();
    model->material->bind();

    shader->set("u_IsInstanced", false);

    shader->set("model", worldMatrix);
    shader->set("view", camera.getViewMatrix());
    shader->set("projection", camera.getProjectionMatrix(aspectRatio));
    shader->set("u_CameraPos", camera.position);

    glBindVertexArray(model->mesh->vao);

    for (GLuint i = 0; i < 4; ++i) {
        glDisableVertexAttribArray(4 + i);
    }

    glDrawElements(GL_TRIANGLES, model->mesh->indexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

bool Renderer::renderObject(const VisibleInstance& instance, const Camera& camera, float aspectRatio) {
    if (!initialized || !instance.object)
        return false;

    const auto& object = *instance.object;

    if (!object.model)
        return true;

    if (!object.model->material || !object.model->mesh)
        return false;

    renderSingle(object.model, instance.worldMatrix, camera, aspectRatio);
    return true;
}

bool Renderer::renderObject(const Object& object, const Camera& camera, float aspectRatio) {
    return renderObject(VisibleInstance{&object, object.getWorldMatrix()}, camera, aspectRatio);
}

void Renderer::renderSkybox(unsigned int cubemapID, const Camera& camera, float aspectRatio) {
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    skyboxShader->use();

    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);

    skyboxShader->set("view", view);
    skyboxShader->set("projection", projection);

    glBindVertexArray(skyboxMesh->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
    skyboxShader->set("skybox", 0);
    skyboxShader->set("exposure", AMBIENT_INTENSITY);

    glDrawElements(GL_TRIANGLES, skyboxMesh->indexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized)
        return false;

    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();
    auto& lightManager = scene.getLightManager();

    const auto dirLights = lightManager.getDirLights();
    const auto pointLights = lightManager.getPointLights();

    renderSkybox(scene.getCubeMap(), camera, aspectRatio);

    processPointLights(pointLights);

    std::unordered_map<const Model*, std::vector<VisibleInstance>> instanceGroups;

    const Frustum& frustum = camera.getFrustum(aspectRatio);

    for (const auto& object : objectManager.getObjects()) {
        if (!object->model)
            continue;

        if (!object->model->mesh || !object->model->material)
            continue;

        glm::mat4 worldMatrix = object->getWorldMatrix();

        if (!object->isVisible(frustum, worldMatrix))
            continue;

        instanceGroups[object->model.get()].push_back(VisibleInstance{object.get(), worldMatrix});
    }

    for (const auto& [modelKey, instances] : instanceGroups) {
        if (instances.empty())
            continue;

        const auto& model = instances.front().object->model;

        auto shader = model->material->getShader();

        if (!shader || !shader->isValid())
            continue;

        shader->use();

        processDirLights(shader, dirLights);

        shader->set("u_ActivePointLightCount", static_cast<int>(pointLights.size()));

        // 기존 Irradiance Map (Diffuse IBL)
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene.getIrradianceMap());
        shader->set("irradianceMap", 8);

        // === 신규: Prefilter Map (Specular IBL) ===
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP, scene.getPrefilterMap()); // Scene 클래스에 getPrefilterMap() 추가 필요
        shader->set("prefilterMap", 9);

        // === 신규: 구워둔 BRDF LUT (2D Texture) ===
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        shader->set("brdfLUT", 10);

        // Max Mipmap Level 유니폼 전달
        shader->set("u_MaxReflectionLOD", 4.0f); // Prefilter Mipmap 최대 레벨에 맞춰 설정

        shader->set("u_AmbientIntensity", AMBIENT_INTENSITY);

        if (instances.size() >= INSTANCE_THRESHOLD) {
            renderInstanced(model, instances, camera, aspectRatio);
        } else {
            for (const auto& inst : instances) {
                renderObject(inst, camera, aspectRatio);
            }
        }
    }

    return true;
}

} // namespace knot