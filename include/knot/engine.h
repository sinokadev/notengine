#pragma once

#include <functional>
#include <string>

#include <knot/window.h>
#include <knot/renderer.h>
#include <knot/manager.h>
#include <knot/camera.hpp>

namespace knot {
    class Engine {
    public:
        using UpdateCallback = std::function<void(Engine&, float)>;

        bool init(int width, int height, const std::string& title, const std::string& assetRoot = "");
        int run();

        ObjectManager& getObjectManager();
        ResourceManager& getResourceManager();
        Camera& getCamera();
        float getDeltaTime() const { return deltaTime; }
        float getAspectRatio() const;

        void setUpdateCallback(UpdateCallback callback);

    private:
        Window window;
        Renderer renderer;
        Camera camera;
        ObjectManager objectManager;
        ResourceManager resourceManager;

        int width = 0;
        int height = 0;
        std::string title;

        bool initialized = false;
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        UpdateCallback updateCallback;

        void update();
        void render();
        void setupCamera();
    };
}
