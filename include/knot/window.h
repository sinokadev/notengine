// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <knot/key.h>

#include <functional>
#include <string>

namespace knot {

/** @brief GLFW-backed OpenGL window and input callback adapter. */
class Window {
public:
    /** @brief Called after the framebuffer dimensions change. */
    using ResizeCallback = std::function<void(int width, int height)>;
    /** @brief Called when a keyboard key changes state. */
    using KeyInputCallback = std::function<void(ScanCode scancode, KeyState action)>;
    /** @brief Called when the cursor position changes. */
    using MousePositionCallback = std::function<void(double x, double y)>;
    /** @brief Called when a mouse button changes state. */
    using MouseButtonCallback = std::function<void(MouseKey mousekey, KeyState action)>;

    /** @brief Constructs an uninitialized window. */
    Window();
    /** @brief Destroys the window and terminates GLFW when necessary. */
    ~Window();

    /** @brief Initializes GLFW and creates a 3.3 core-profile window.
     *  @return true if the window is created successfully. */
    bool init(int width, int height, const std::string& title);
    /** @brief Releases the GLFW window, callbacks, and GLFW state. */
    void shutdown();

    /** @brief Makes this window's OpenGL context current.
     *  @return false if no window has been created. */
    bool active();
    /** @brief Swaps buffers and polls GLFW events. */
    void loop();

    /** @brief Reports whether GLFW has requested window closure. */
    bool isClose() const;
    /** @brief Enables buffer swaps synchronized to the display refresh rate. */
    void enableVsync();
    /** @brief Disables buffer-swap synchronization. */
    void disableVsync();

    /** @brief Returns the GLAD-compatible GLFW procedure loader. */
    GLADloadfunc getProcAddress() const;

    /** @brief Returns the current framebuffer width in pixels. */
    int getFramebufferWidth() const {
        return framebufferWidth;
    }
    /** @brief Returns the current framebuffer height in pixels. */
    int getFramebufferHeight() const {
        return framebufferHeight;
    }
    /** @brief Returns the underlying GLFW handle, or nullptr before init(). */
    GLFWwindow* getHandle() const {
        return windowHandle;
    }

    /** @brief Sets the framebuffer resize callback. */
    void setResizeCallback(ResizeCallback callback);
    /** @brief Sets the keyboard input callback. */
    void setKeyInputCallback(KeyInputCallback callback);
    /** @brief Sets the cursor-position callback. */
    void setMousePositionCallback(MousePositionCallback callback);
    /** @brief Sets the mouse-button callback. */
    void setMouseButtonCallback(MouseButtonCallback callback);

private:
    GLFWwindow* windowHandle = nullptr;
    bool initialized = false;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    ResizeCallback resizeCallback;
    KeyInputCallback keyInputCallback;
    MousePositionCallback mousePositionCallback;
    MouseButtonCallback mouseButtonCallback;

    static void framebufferSizeCallback_glfw(GLFWwindow* window, int width, int height);
    static void keyCallback_glfw(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorPositionCallback_glfw(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback_glfw(GLFWwindow* window, int button, int action, int mods);
};
} // namespace knot
