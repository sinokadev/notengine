#include <knot/renderer.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cassert>

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

    initialized = true;
    return true;
}

void Renderer::beginFrame(int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth <= 0 || framebufferHeight <= 0) return;
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void Renderer::processDirLights(const std::shared_ptr<Shader>& shader, const std::vector<const DirLight*>& dirLights) {
    if (!shader) return;

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

bool Renderer::renderObject(const Object& object, const Camera& camera, float aspectRatio) {
    if (!initialized) return false;

    if (dynamic_cast<const Camera*>(&object) || 
        dynamic_cast<const PbrPointLight*>(&object) || 
        dynamic_cast<const DirLight*>(&object)) {
        return true;
    }

    if (!object.material) return false;
    const auto shader = object.material->getShader();
    if (!shader || !shader->isValid()) return false;

    object.material->bind();

    shader->set("view", camera.getViewMatrix());
    shader->set("projection", glm::perspective(glm::radians(camera.fov), aspectRatio, kNearPlane, kFarPlane));
    shader->set("model", object.getWorldMatrix());
    shader->set("u_CameraPos", camera.position);

    if (!object.mesh || !object.mesh->isReady()) return false;

    glBindVertexArray(object.mesh->vao);
    glDrawElements(GL_TRIANGLES, object.mesh->indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return true;
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized) return false;

    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();

    std::vector<const DirLight*> localDirLights;
    std::vector<const PbrPointLight*> localPointLights;
    
    for (const auto& object : objectManager.getObjects()) {
        if (const auto* dl = dynamic_cast<const DirLight*>(object.get())) {
            localDirLights.push_back(dl);
        } else if (const auto* pbrl = dynamic_cast<const PbrPointLight*>(object.get())) {
            localPointLights.push_back(pbrl);
        }
    }

    processPointLights(localPointLights);

    for (const auto& object : objectManager.getObjects()) {
        if (!object->material) continue;
        const auto shader = object->material->getShader();
        
        if (shader && shader->isValid()) {
            shader->use();
            processDirLights(shader, localDirLights);
            shader->set("u_ActivePointLightCount", static_cast<int>(localPointLights.size()));
        }

        renderObject(*object, camera, aspectRatio);
    }

    return true;
}

} // namespace knot