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

    ObjectManager& getObjectManager();
    ResourceManager& getResourceManager();
    Camera& getCamera();

    void setUpdateCallback(UpdateCallback callback);

    void update(float dt);

    Object& getMainCameraObject();
    void setMainCameraObject(Object& obj);

private:
    ObjectManager objectManager;
    ResourceManager resourceManager;

    UpdateCallback updateCallback;

    Camera* camera = nullptr;
    Object* mainCameraObj = nullptr;

    void setupCamera();
};
} // namespace knot