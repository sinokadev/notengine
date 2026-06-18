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

    // 1. Scene에서 카메라와 오브젝트 매니저를 가져옵니다.
    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();

    // 2. Scene 내부의 모든 오브젝트를 순회하며 기존 renderObject를 호출합니다.
    for (const auto& object : objectManager.getObjects()) {
        renderObject(object, camera, aspectRatio);
    }

    return true;
}

} // namespace knot
