// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <functional>
#include <string>

#include <knot/renderer.h>
#include <knot/window.h>
#include <knot/scene.h>
#include <knot/event.h>

namespace knot {
struct TimerTask {
    std::function<void()> callback;
    float time;
    float interval; // ms
    int id;
};

class Engine {
public:
    using RenderLoopCallback = std::function<void(float)>;

    bool init(int width, int height, const std::string& title, const std::string& assetRoot = "");
    int run();

    void setRenderLoopCallback(RenderLoopCallback callback) {
        renderLoopCallback = std::move(callback);
    }

    Window& getWindow();
    float getDeltaTime() const {
        return deltaTime;
    }
    float getAspectRatio() const;

    bool setScene(Scene& s);
    void setClearColor(float r, float g, float b, float a) {
        clearColor = {r, g, b, a};
    }

    void setEventCallback(std::function<void(Event&)> callback) {
        eventCallback = std::move(callback);
    }

    void pushUserEvent(uint32_t userCode, std::any data = {}) {
        knot::Event event;
        event.type = knot::EventType::User;
        event.userCode = userCode;
        event.userData = std::move(data);

        // 등록된 콜백으로 이벤트 즉시 전송 (또는 큐에 넣어 연기 처리)
        if (eventCallback) {
            eventCallback(event);
        }
    }

    int after(float time, std::function<void()> callback);
    int repeat(float time, std::function<void()> callback);

private:
    Window window;
    Renderer renderer;

    glm::vec4 clearColor = {0, 0, 0, 1};

    int width = 0;
    int height = 0;
    std::string title;

    bool initialized = false;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    Scene* scene = nullptr;

    void update();
    void render();
    void processTimer();

    std::function<void(Event&)> eventCallback = nullptr;
    
    RenderLoopCallback renderLoopCallback;

    std::vector<TimerTask> afterTimerTasks;
    std::vector<TimerTask> repeatTimerTasks;
    int nextTimerTaskId;
};
} // namespace knot
