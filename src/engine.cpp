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

    window.setKeyInputCallback([this](ScanCode code, KeyState action) {
        if (this->eventCallback) {
            // 임시로 구조체/클래스를 만들어 마스터 콜백에 전달
            // (knot::Event 정의에 맞게 생성자를 호출하시면 됩니다)
            knot::Event e;
            e.type = knot::KeyInput;
            e.key = code;
            e.action = action;
            this->eventCallback(e);
        }
    });

    // 3. 마우스 이동 콜백 -> 엔진 마스터 이벤트 콜백으로 포장 및 전달
    window.setMousePositionCallback([this](double xpos, double ypos) {
        if (this->eventCallback) {
            knot::Event e;
            e.type = knot::MouseMoved; // 또는 설정하신 enum 타입명
            e.x = xpos; // MouseMovedEvent 같은 상속 구조라면 캐스팅 혹은 다형성 활용
            e.y = ypos;
            this->eventCallback(e);
        }
    });

    // 4. 마우스 버튼 콜백 -> 엔진 마스터 이벤트 콜백으로 포장 및 전달
    window.setMouseButtonCallback([this](MouseKey button, KeyState action) {
        if (this->eventCallback) {
            knot::Event e;
            e.type = knot::MouseButton; 
            e.mouseButton = button;
            e.action = action;
            this->eventCallback(e);
        }
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
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
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
