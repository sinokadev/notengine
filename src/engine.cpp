#include <knot/engine.h>
#include <knot/version.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <iostream>

namespace knot {

Engine::~Engine() {
    shutdown();
}

bool Engine::init(int width, int height, const std::string& title, const std::string& assetRoot) {
    this->width = width;
    this->height = height;
    this->title = title;

    std::cout << "[Info] Not Engine v"
            << GAME_VERSION_STRING
            << std::endl;

    std::cout << "[Info] Init..."
            << std::endl;
            
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
        shutdown();
        return false;
    }

    if (!renderer.init(window.getProcAddress())) {
        shutdown();
        return false;
    }

    window.setResizeCallback([this](int framebufferWidth, int framebufferHeight) {
        this->width = framebufferWidth;
        this->height = framebufferHeight;
    });

    window.setKeyInputCallback([this](ScanCode code, KeyState action) {
        if (this->eventCallback) {
            knot::Event e;
            e.type = knot::KeyInput;
            e.key = code;
            e.action = action;
            this->eventCallback(e);
        }
    });

    window.setMousePositionCallback([this](double xpos, double ypos) {
        if (this->eventCallback) {
            knot::Event e;
            e.type = knot::MouseMoved;
            e.x = xpos;
            e.y = ypos;
            this->eventCallback(e);
        }
    });

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

    while (!window.isClose() && !shouldQuit) {
        const double currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        update();
        processTimer();
        render();
        window.loop();
    }

    shutdown();

    return 0;
}

void Engine::shutdown() {
    if (!initialized) {
        return;
    }

    // 1. Scene 자원 정리 (Scene 내 텍스처, 셰이더, 오브젝트 등 OpenGL 리소스 해제)
    if (scene) {
        scene->shutdown();
        scene = nullptr;
    }

    // 2. Renderer 자원 정리 (SSBO, VBO, Skybox 등 OpenGL 리소스 해제)
    renderer.shutdown();

    // 3. 타이머 및 콜백 정리
    eventCallback = nullptr;
    renderLoopCallback = nullptr;
    afterTimerTasks.clear();
    repeatTimerTasks.clear();

    // 4. 모든 GPU 리소스 해제 후 마지막으로 Window 및 OpenGL Context 셧다운
    window.shutdown();

    initialized = false;
}

void Engine::quit() {
    shouldQuit = true;
}

void Engine::update() {
    scene->update(deltaTime);
}

void Engine::render() {
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer.beginFrame(window.getFramebufferWidth(), window.getFramebufferHeight());
    renderer.renderScene(*scene, getAspectRatio());
    if (renderLoopCallback) {
        renderLoopCallback(deltaTime);
    }
}

void Engine::processTimer() {
    // after
    for (int i = 0; i < afterTimerTasks.size();) {
        afterTimerTasks[i].time -= deltaTime * 1000;
        if (afterTimerTasks[i].time <= 0.0f) {
            pushUserEvent(afterTimerTasks[i].eventCode, afterTimerTasks[i].eventData);
            afterTimerTasks.erase(afterTimerTasks.begin() + i);
        } else {
            i++;
        }
    }

    // repeat
    for (int i = 0; i < repeatTimerTasks.size(); i++) {
        repeatTimerTasks[i].time -= deltaTime * 1000;
        if (repeatTimerTasks[i].time <= 0.0f) {
            pushUserEvent(repeatTimerTasks[i].eventCode, repeatTimerTasks[i].eventData);
            repeatTimerTasks[i].time += repeatTimerTasks[i].interval;
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

Window& Engine::getWindow() {
    return window;
}

bool Engine::setScene(Scene& s) {
    scene = &s;
    return true;
}

int Engine::after(double interval, uint32_t eventCode, std::any eventData) {
    TimerTask afterTimerTask;
    afterTimerTask.time = interval;
    afterTimerTask.interval = interval;
    afterTimerTask.eventCode = eventCode;
    afterTimerTask.eventData = eventData;
    afterTimerTask.id = nextTimerTaskId++;
    afterTimerTasks.push_back(afterTimerTask);

    return afterTimerTask.id;
}

int Engine::repeat(double interval, uint32_t eventCode, std::any eventData) {
    TimerTask repeatTimerTask;
    repeatTimerTask.time = interval;
    repeatTimerTask.interval = interval;
    repeatTimerTask.eventCode = eventCode;
    repeatTimerTask.eventData = eventData;
    repeatTimerTask.id = nextTimerTaskId++;
    repeatTimerTasks.push_back(repeatTimerTask);

    return repeatTimerTask.id;
}

} // namespace knot
