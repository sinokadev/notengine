#include <knot/renderer.h>

#include <glad/gl.h>

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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

    initialized = true;
    return true;
}

void Renderer::beginFrame(int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

bool Renderer::renderObject(const Object& object, const Camera& camera, float aspectRatio) {
    if (!initialized) {
        return false;
    }

    if (dynamic_cast<const Camera*>(&object)) {
        return true;
    }

    if (dynamic_cast<const PongPointLight*>(&object)) {
        return true; 
    }

    if (dynamic_cast<const PongDirLight*>(&object)) {
        return true;
    }

    if (!object.material) {
        std::cerr << "[Error] Object ID " << object.id << " has no material" << std::endl;
        return false;
    }

    const auto shader = object.material->getShader();
    if (!shader || !shader->isValid()) {
        std::cerr << "[Error] Object ID " << object.id << " has no valid shader" << std::endl;
        return false;
    }

    object.material->bind();

    shader->set("view", camera.getViewMatrix());
    shader->set("projection", glm::perspective(glm::radians(camera.fov), aspectRatio, kNearPlane, kFarPlane));
    shader->set("model", object.getWorldMatrix());
    shader->set("viewPos", camera.position);

    // Set Directional Light uniforms
    if (!activeDirLights.empty()) {
        const auto* dirLight = activeDirLights.front();
        shader->set("dirLight.direction", dirLight->direction);
        shader->set("dirLight.ambient",   dirLight->ambient);
        shader->set("dirLight.diffuse",   dirLight->diffuse);
        shader->set("dirLight.specular",  dirLight->specular);
    } else {
        shader->set("dirLight.direction", glm::vec3(0.0f, -1.0f, 0.0f));
        shader->set("dirLight.ambient",   glm::vec3(0.0f));
        shader->set("dirLight.diffuse",   glm::vec3(0.0f));
        shader->set("dirLight.specular",  glm::vec3(0.0f));
    }

    // Set Point Light uniforms
    for (int i = 0; i < 1; ++i) { // POINT_LIGHT_COUNT is 1
        std::string prefix = "pointLights[" + std::to_string(i) + "].";
        if (i < static_cast<int>(activePointLights.size())) {
            const auto* pointLight = activePointLights[i];
            shader->set(prefix + "position", pointLight->position);
            shader->set(prefix + "ambient",  pointLight->ambient);
            shader->set(prefix + "diffuse",  pointLight->diffuse);
            shader->set(prefix + "specular", pointLight->specular);
            shader->set(prefix + "constant", pointLight->constant);
            shader->set(prefix + "linear",   pointLight->linear);
            shader->set(prefix + "quadratic",pointLight->quadratic);
        } else {
            // Safe default to avoid division by zero and contribution
            shader->set(prefix + "position", glm::vec3(0.0f));
            shader->set(prefix + "ambient",  glm::vec3(0.0f));
            shader->set(prefix + "diffuse",  glm::vec3(0.0f));
            shader->set(prefix + "specular", glm::vec3(0.0f));
            shader->set(prefix + "constant", 1.0f);
            shader->set(prefix + "linear",   0.0f);
            shader->set(prefix + "quadratic",0.0f);
        }
    }

    if (!object.mesh || !object.mesh->isReady()) {
        std::cerr << "[Error] Object ID " << object.id << " has no valid mesh" << std::endl;
        return false;
    }

    glBindVertexArray(object.mesh->vao);
    glDrawElements(GL_TRIANGLES, object.mesh->indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return true;
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized) {
        return false;
    }

    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();

    activeDirLights.clear();
    activePointLights.clear();
    for (const auto& object : objectManager.getObjects()) {
        if (const auto* dl = dynamic_cast<const PongDirLight*>(object.get())) {
            activeDirLights.push_back(dl);
        } else if (const auto* pl = dynamic_cast<const PongPointLight*>(object.get())) {
            activePointLights.push_back(pl);
        }
    }

    for (const auto& object : objectManager.getObjects()) {
        renderObject(*object, camera, aspectRatio);
    }

    return true;
}

} // namespace knot
