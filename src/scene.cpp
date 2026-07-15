#include <knot/scene.h>
#include <glad/gl.h>

#include <knot/utility.h>

namespace knot {
Scene::Scene() {
    if (glad_glCreateShader != nullptr) {
        resourceManager.init();
    }
}

void Scene::loadHDRMap(const std::string& path) {
    if (hdrMap != 0) glDeleteTextures(1, &hdrMap);
    if (cubeMap != 0) glDeleteTextures(1, &cubeMap);

    hdrMap = loadHDRTexture(path); 
    
    cubeMap = bakeHDRMapToCubemap(hdrMap, 512); 
}

Object& Scene::getMainCameraObject() {
    return *mainCameraObj;
}
void Scene::setMainCameraObject(Object& obj) {
    mainCameraObj = &obj;
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
} // namespace knot