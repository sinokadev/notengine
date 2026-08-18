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

    while (!window.isClose()) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        update();
        processTimer();
        render();
        window.loop();
    }

    return 0;
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
    for (int i=0; i < afterTimerTasks.size();) {
        afterTimerTasks[i].time-=deltaTime;
        if (afterTimerTasks[i].time <= 0.0f) {
            afterTimerTasks[i].callback();
            afterTimerTasks.erase(afterTimerTasks.begin() + i);
        } else {
            i++;
        }
    }

    // repeat
    for (int i=0; i < repeatTimerTasks.size(); i++) {
        repeatTimerTasks[i].time-=deltaTime;
        if (repeatTimerTasks[i].time <= 0.0f) {
            repeatTimerTasks[i].callback();
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

int Engine::after(float delay, std::function<void()> callback) {
    TimerTask afterTimerTask;
    afterTimerTask.time = delay;
    afterTimerTask.interval = delay;
    afterTimerTask.callback = callback;
    afterTimerTask.id = nextTimerTaskId++;
    afterTimerTasks.push_back(afterTimerTask);

    return afterTimerTask.id;
}

int Engine::repeat(float interval, std::function<void()> callback) {
    TimerTask repeatTimerTask;
    repeatTimerTask.time = interval;
    repeatTimerTask.interval = interval;
    repeatTimerTask.callback = callback;
    repeatTimerTask.id = nextTimerTaskId++;
    repeatTimerTasks.push_back(repeatTimerTask);

    return repeatTimerTask.id;
}

} // namespace knot
