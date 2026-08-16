#include <knot/renderer.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cassert>

#define AMBIENT_INTENSITY 5.0f

namespace knot {

bool Renderer::init(GLADloadfunc loadProc) {
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

    initialized = true;
    return true;
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
        shader->set("dirLight.direction", dirLight->direction);
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

void Renderer::renderInstanced(
    const std::shared_ptr<Model>& model,
    const std::vector<VisibleInstance>& instances,
    const Camera& camera,
    float aspectRatio
) {
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
        instanceData.push_back({
            inst.worldMatrix
        });
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        instanceData.size() * sizeof(InstanceData),
        instanceData.data(),
        GL_STREAM_DRAW
    );

    model->material->bind();

    shader->set("view", camera.getViewMatrix());
    shader->set("projection", camera.getProjectionMatrix(aspectRatio));
    shader->set("u_CameraPos", camera.position);

    model->mesh->setupInstanceAttributes(instanceVBO);

    glBindVertexArray(model->mesh->vao);

    glDrawElementsInstanced(
        GL_TRIANGLES,
        model->mesh->indexCount,
        GL_UNSIGNED_INT,
        nullptr,
        static_cast<GLsizei>(instanceData.size())
    );

    glBindVertexArray(0);
}

bool Renderer::renderObject(
    const VisibleInstance& instance,
    const Camera& camera,
    float aspectRatio
) {
    if (!initialized || !instance.object)
        return false;

    const auto& object = *instance.object;

    if (!object.model)
        return true;

    if (!object.model->material || !object.model->mesh)
        return false;

    renderInstanced(object.model, { instance }, camera, aspectRatio);
    return true;
}

bool Renderer::renderObject(
    const Object& object,
    const Camera& camera,
    float aspectRatio
) {
    return renderObject(VisibleInstance{ &object, object.getWorldMatrix() }, camera, aspectRatio);
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

    std::vector<const DirLight*> localDirLights;
    std::vector<const PbrPointLight*> localPointLights;

    renderSkybox(scene.getCubeMap(), camera, aspectRatio);

    for (const auto& object : objectManager.getObjects()) {
        if (const auto* dl = dynamic_cast<const DirLight*>(object.get())) {
            localDirLights.push_back(dl);
        } else if (const auto* pbrl = dynamic_cast<const PbrPointLight*>(object.get())) {
            localPointLights.push_back(pbrl);
        }
    }

    processPointLights(localPointLights);

    std::unordered_map<
        const Model*,
        std::vector<VisibleInstance>
    > instanceGroups;

    const Frustum& frustum = camera.getFrustum(aspectRatio);

    for (const auto& object : objectManager.getObjects()) {
        if (!object->model)
            continue;

        if (!object->model->mesh || !object->model->material)
            continue;

        glm::mat4 worldMatrix = object->getWorldMatrix();
        if (!object->isVisible(frustum, worldMatrix))
            continue;

        instanceGroups[object->model.get()].push_back(VisibleInstance{
            object.get(),
            worldMatrix
        });
    }

    for (const auto& [modelKey, instances] : instanceGroups) {
        if (instances.empty())
            continue;

        const auto& model = instances.front().object->model;

        auto shader = model->material->getShader();

        if (!shader || !shader->isValid())
            continue;

        shader->use();

        processDirLights(shader, localDirLights);

        shader->set(
            "u_ActivePointLightCount",
            static_cast<int>(localPointLights.size())
        );

        glActiveTexture(GL_TEXTURE8);
        glBindTexture(
            GL_TEXTURE_CUBE_MAP,
            scene.getIrradianceMap()
        );

        shader->set("irradianceMap", 8);
        shader->set("u_AmbientIntensity", AMBIENT_INTENSITY);

        if (instances.size() >= INSTANCE_THRESHOLD) {
            renderInstanced(
                model,
                instances,
                camera,
                aspectRatio
            );
        } else {
            for (const auto& inst : instances) {
                renderObject(
                    inst,
                    camera,
                    aspectRatio
                );
            }
        }
    }

    return true;
}

} // namespace knot