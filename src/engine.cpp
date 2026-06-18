#include <knot/engine.h>

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

    window.setResizeCallback([this](int framebufferWidth, int framebufferHeight) {
        this->width = framebufferWidth;
        this->height = framebufferHeight;
    });

    renderer.beginFrame(window.getFramebufferWidth(), window.getFramebufferHeight());

    window.enableVsync();
    initialized = true;
    return true;
}

int Engine::run() {
    if (!initialized) {
        std::cerr << "[Error] Engine is not initialized" << std::endl;
        return 1;
    }

    if (!scene) {
        std::cerr << "[Error] Engine cannot run without a Scene registered." << std::endl;
        return 1;
    }

    while (!window.isClose()) {
        update();
        render();
        window.loop();
    }

    return 0;
}

void Engine::update() {
    const float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    scene->update(deltaTime);
}

void Engine::render() {
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer.beginFrame(window.getFramebufferWidth(), window.getFramebufferHeight());
    renderer.renderScene(*scene, getAspectRatio());
}

float Engine::getAspectRatio() const {
    const int framebufferWidth = window.getFramebufferWidth();
    const int framebufferHeight = window.getFramebufferHeight();
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return 1.0f;
    }

    return static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
}

Window& Engine::getWindow() {
    return window;
}

bool Engine::setScene(Scene& s) {
    scene = &s;
    return true;
}
} // namespace knot
