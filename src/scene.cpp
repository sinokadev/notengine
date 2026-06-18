#include <knot/scene.h>

namespace knot {
void Scene::setupCamera() {
    camera.position = glm::vec3(0.0f, 0.0f, 3.0f);
    camera.lookAtTarget(glm::vec3(0.0f, 0.0f, 0.0f));
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
    return camera;
}

void Scene::update(float dt) {
    if (updateCallback) {
        updateCallback(*this, dt); 
    }
}
}