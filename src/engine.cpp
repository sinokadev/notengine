#include <knot/engine.h>
#include <knot/scene.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <iostream>

namespace knot {

bool Engine::init(int width, int height, const std::string& title, const std::string& assetRoot) {
    this->width = width;
    this->height = height;
    this->title = title;

    if (!assetRoot.empty()) {
        setAssetRoot(assetRoot);
    } else {
#ifdef NOTENGINE_ASSET_ROOT
        setAssetRoot(NOTENGINE_ASSET_ROOT);
#endif
    }

    if (!window.init(width, height, title)) {
        return false;
    }

    if (!window.active()) {
        window.shutdown();
        return false;
    }

    if (!renderer.init(window.getProcAddress())) {
        window.shutdown();
        return false;
    }

    if (!resourceManager.init()) {
        window.shutdown();
        return false;
    }

    window.setResizeCallback([this](int framebufferWidth, int framebufferHeight) {
        this->width = framebufferWidth;
        this->height = framebufferHeight;
    });
    
    renderer.beginFrame(window.getFramebufferWidth(), window.getFramebufferHeight());

    window.enableVsync();
    initialized = true;
    return true;
}

Engine& Engine::get() {
    static Engine instance; 
    return instance;
}

int Engine::run() {
    if (!initialized) {
        return 1;
    }

    if (!activeScene) {
        return 1;
    }

    while (!window.isClose()) {
        update();
        render();
        window.loop();
    }

    if (activeScene) {
        activeScene->onExit();
    }

    return 0;
}

void Engine::update() {
    const float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (activeScene) {
        activeScene->onUpdate(deltaTime);
    }
}

void Engine::render() {
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const int framebufferWidth = window.getFramebufferWidth();
    const int framebufferHeight = window.getFramebufferHeight();
    renderer.beginFrame(framebufferWidth, framebufferHeight);

    const float aspectRatio = getAspectRatio();

    if (activeScene) {
        auto& objectManager = activeScene->getObjectManager();
        auto& camera = activeScene->getCamera();

        for (const auto& object : objectManager.getObjects()) {
            renderer.renderObject(object, camera, aspectRatio);
        }
    }
}

float Engine::getAspectRatio() const {
    const int framebufferWidth = window.getFramebufferWidth();
    const int framebufferHeight = window.getFramebufferHeight();
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return 1.0f;
    }

    return static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
}

ResourceManager& Engine::getResourceManager() {
    return resourceManager;
}

Window& Engine::getWindow() {
    return window;
}

} // namespace knot