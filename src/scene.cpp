#include <knot/scene.h>
#include <glad/gl.h>

namespace knot {
Scene::Scene() {
    if (glad_glCreateShader != nullptr) {
        resourceManager.init();
    }
    setupCamera();
}

void Scene::setupCamera() {
    auto& movingCam = objectManager.createMovingCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    camera = &movingCam;
    mainCameraObj = &movingCam;
    camera->lookAtTarget(glm::vec3(0.0f, 0.0f, 0.0f));
}

void Scene::setUpdateCallback(UpdateCallback callback) {
    updateCallback = std::move(callback);
}

ObjectManager& Scene::getObjectManager() {
    return objectManager;
}

ResourceManager& Scene::getResourceManager() {
    return resourceManager;
}

Camera& Scene::getCamera() {
    if (mainCameraObj) {
        auto* cam = dynamic_cast<Camera*>(mainCameraObj);
        if (cam) {
            return *cam;
        }
    }
    return *camera;
}

void Scene::update(float dt) {
    if (updateCallback) {
        updateCallback(*this, dt); 
    }
}
}