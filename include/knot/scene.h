// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <functional>
#include <string>

#include <knot/camera.h>
#include <knot/manager.h>

namespace knot {

class Scene {
public:
    using UpdateCallback = std::function<void(Scene&, float)>;

    Scene();
    ~Scene();

    void clear();
    void shutdown();

    ObjectManager& getObjectManager();
    LightManager& getLightManager();
    ResourceManager& getResourceManager();

    Camera& getCamera();
    const Camera& getCamera() const;
    void setCamera(Camera& cam);

    void setUpdateCallback(UpdateCallback callback);

    void update(float dt);

    void loadHDRMap(const std::string& path);

    unsigned int getCubeMap() const {
        return cubeMap;
    }

    unsigned int getIrradianceMap() const {
        return irradianceMap;
    }

private:
    ObjectManager objectManager;
    LightManager lightManager;
    ResourceManager resourceManager;

    UpdateCallback updateCallback;

    Camera* camera = nullptr;

    unsigned int cubeMap = 0;
    unsigned int irradianceMap = 0;
};

} // namespace knot