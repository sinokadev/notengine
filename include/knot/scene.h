#pragma once

#include <knot/camera.h>
#include <knot/manager.h>

namespace knot {

class Scene {
public:
    Scene() = default;
    
    virtual ~Scene() = default;
    virtual void onEnter() = 0;
    virtual void onUpdate(float deltaTime) = 0;
    virtual void onExit() = 0;

    ObjectManager& getObjectManager() { return objectManager; }
    Camera& getCamera() { return *camera; }

protected:
    ObjectManager objectManager;
    std::unique_ptr<Camera> camera = nullptr;
};

} // namespace knot