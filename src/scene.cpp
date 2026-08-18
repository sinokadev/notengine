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
    if (cubeMap != 0)
        glDeleteTextures(1, &cubeMap);
    if (irradianceMap != 0)
        glDeleteTextures(1, &irradianceMap);

    unsigned int tempHdrMap = loadHDRTexture(path);

    cubeMap = bakeHDRMapToCubemap(tempHdrMap, 512);

    irradianceMap = bakeCubemapToIrradianceMap(cubeMap, 32);

    glDeleteTextures(1, &tempHdrMap);
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

const Camera& Scene::getCamera() const {
    return camera;
}

void Scene::setCamera(const Camera& cam) {
    camera = cam;
}

void Scene::update(float dt) {
    if (updateCallback) {
        updateCallback(*this, dt);
    }
}
} // namespace knot