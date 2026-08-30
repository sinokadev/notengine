// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <functional>
#include <string>

#include <knot/renderer.h>
#include <knot/window.h>
#include <knot/scene.h>
#include <knot/event.h>

namespace knot {
/**
 * @brief Represents a scheduled user event.
 *
 * Timer tasks are internally used by Engine::after() and Engine::repeat()
 * to schedule user events.
 */
struct TimerTask {
    /**
     * @brief The application-defined event code.
     */
    uint32_t eventCode;

    /**
     * @brief Data passed to the event callback when the timer fires.
     */
    std::any eventData;

    /**
     * @brief Time remaining until the next event trigger, in milliseconds.
     */
    double time;

    /**
     * @brief The interval between repeated triggers, in milliseconds.
     */
    double interval;

    /**
     * @brief The unique identifier of the timer task.
     */
    int id;
};

/**
 * @brief This is the master class for the not engine.
 *
 * It manages the game loop and provides related functionalities,
 * primarily handling application window management, renderer management, scene rendering, event handling, and timer tasks.
 *
 * An Engine must be initialized with init() and have a Scene registered
 * with setScene() before run() is called.
 */
class Engine {
public:
    /**
     * @brief Callback invoked once per frame.
     *
     * The callback is executed after the scene has been rendered.
     *
     * @param deltaTime The elapsed time since the previous frame, in seconds.
     */
    using RenderLoopCallback = std::function<void(double)>;

    Engine() = default;

    /**
     * @brief Destroys the engine.
     *
     * Automatically calls shutdown() to release engine resources.
     */
    ~Engine();

    /**
     * @brief Initializes the engine.
     *
     * Initializes the application window and renderer, configures input
     * callbacks, enables VSync, and prepares the engine for the main loop.
     *
     * If assetRoot is empty, the engine uses the default asset root
     * configured at compile time when available.
     *
     * @param width The initial window width in pixels.
     * @param height The initial window height in pixels.
     * @param title The title of the application window.
     * @param assetRoot The root directory used to locate engine assets.
     *
     * @return true if initialization succeeds, false otherwise.
     */
    bool init(int width, int height, const std::string& title, const std::string& assetRoot = "");

    /**
     * @brief Runs the main engine loop.
     *
     * The engine updates the active scene, processes timer tasks,
     * renders the scene, and processes window events once per frame.
     *
     * A Scene must be registered with setScene() before calling this method.
     *
     * @return 0 if the engine exits normally, or 1 if the engine was
     * not initialized or no scene was registered.
     */
    int run();

    /**
     * @brief Shuts down the engine.
     *
     * Releases scene resources, renderer resources, timer tasks,
     * callbacks, and finally the window and OpenGL context.
     *
     * Calling shutdown() on an already shut down engine has no effect.
     *
     * This function executes automatically before run() terminates, so there is no need to run it manually.
     */
    void shutdown();

    /**
     * @brief Sets the callback executed during the render loop.
     *
     * The callback is called after the active scene has been rendered.
     *
     * @param callback The callback to execute once per frame.
     */
    void setRenderLoopCallback(RenderLoopCallback callback) {
        renderLoopCallback = std::move(callback);
    }

    /**
     * @brief Gets the application window.
     *
     * @return A reference to the engine's Window instance.
     */
    Window& getWindow();

    /**
     * @brief Gets the elapsed time since the previous frame.
     *
     * @return The frame delta time in seconds.
     */
    float getDeltaTime() const {
        return deltaTime;
    }

    /**
     * @brief Gets the current framebuffer aspect ratio.
     *
     * The aspect ratio is calculated from the framebuffer dimensions
     * rather than the logical window size.
     *
     * @return The framebuffer width-to-height ratio.
     *
     * @retval 1.0f If the framebuffer width or height is zero or negative.
     */
    float getAspectRatio() const;

    /**
     * @brief Sets the active scene.
     *
     * The scene is updated and rendered during the engine's main loop.
     *
     * @param s The scene to register as the active scene.
     *
     * @return true after the scene has been registered.
     */
    bool setScene(Scene& s);

    /**
     * @brief Sets the renderer clear color.
     *
     * The specified color is used to clear the color buffer at the
     * beginning of each frame.
     *
     * @param r Red component in the range [0, 1].
     * @param g Green component in the range [0, 1].
     * @param b Blue component in the range [0, 1].
     * @param a Alpha component in the range [0, 1].
     */
    void setClearColor(float r, float g, float b, float a) {
        clearColor = {r, g, b, a};
    }

    /**
     * @brief Sets the event callback.
     *
     * The callback receives keyboard, mouse, and user-defined events
     * generated by the engine.
     *
     * @param callback The callback used to handle events.
     */
    void setEventCallback(std::function<void(Event&)> callback) {
        eventCallback = std::move(callback);
    }

    /**
     * @brief Pushes a user-defined event.
     *
     * Creates an event of type EventType::User and immediately invokes
     * the registered event callback if one exists.
     *
     * @param userCode The application-defined event code.
     * @param data Optional data associated with the event.
     */
    void pushUserEvent(uint32_t userCode, std::any data = {}) {
        knot::Event event;
        event.type = knot::EventType::User;
        event.userCode = userCode;
        event.userData = std::move(data);

        if (eventCallback) {
            eventCallback(event);
        }
    }

    /**
     * @brief Schedules a one-time user event.
     *
     * The event is triggered once after the specified interval.
     * The interval is measured in milliseconds.
     *
     * @param interval The delay before the event is triggered, in milliseconds.
     * @param eventCode The application-defined event code.
     * @param eventData Optional data associated with the event.
     *
     * @return The unique ID assigned to the timer task.
     */
    int after(double interval, uint32_t eventCode, std::any eventData = {});

    /**
     * @brief Schedules a repeating user event.
     *
     * The event is triggered repeatedly using the specified interval.
     * The interval is measured in milliseconds.
     *
     * @param interval The time between event triggers, in milliseconds.
     * @param eventCode The application-defined event code.
     * @param eventData Optional data associated with the event.
     *
     * @return The unique ID assigned to the timer task.
     */
    int repeat(double interval, uint32_t eventCode, std::any eventData = {});

    /**
     * @brief Requests the engine to stop running.
     *
     * The main loop exits after the current iteration finishes.
     */
    void quit();

private:
    Window window;

    glm::vec4 clearColor = {0, 0, 0, 1};

    int width = 0;
    int height = 0;
    std::string title;
    bool shouldQuit = false;

    bool initialized = false;
    double deltaTime = 0.0f;
    double lastFrame = 0.0f;

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
