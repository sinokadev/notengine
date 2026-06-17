#pragma once

#include <string>
#include <memory>

#include <knot/manager.h>
#include <knot/renderer.h>
#include <knot/window.h>

namespace knot {

class Scene; 

class Engine {
public:
    bool init(int width, int height, const std::string& title, const std::string& assetRoot = "");
    int run();

    ResourceManager& getResourceManager();
    Window& getWindow();
    
    float getDeltaTime() const {
        return deltaTime;
    }
    float getAspectRatio() const;

    template <typename T, typename... Args>
    void changeScene(Args&&... args) {
        if (activeScene) {
            activeScene->onExit();
        }
        
        activeScene = std::make_unique<T>(std::forward<Args>(args)...);
        activeScene->onEnter();
    }

    Scene* getActiveScene() const { return activeScene.get(); }

    static Engine& get();

private:
    Window window;
    Renderer renderer;
    ResourceManager resourceManager;

    int width = 0;
    int height = 0;
    std::string title;

    bool initialized = false;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    std::unique_ptr<Scene> activeScene = nullptr;

    void update();
    void render();
};
} // namespace knot