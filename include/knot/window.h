// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <knot/key.h>

#include <functional>
#include <string>

namespace knot {
class Window {
public:
    using ResizeCallback = std::function<void(int width, int height)>;
    using KeyInputCallback = std::function<void(ScanCode scancode, KeyState action)>;
    using MousePositionCallback = std::function<void(double x, double y)>;
    using MouseButtonCallback = std::function<void(MouseKey mousekey, KeyState action)>;

    Window();
    ~Window();

    bool init(int width, int height, const std::string& title);
    void shutdown();

    bool active();
    void loop();

    bool isClose() const;
    void enableVsync();

    GLADloadfunc getProcAddress() const;

    int getFramebufferWidth() const {
        return framebufferWidth;
    }
    int getFramebufferHeight() const {
        return framebufferHeight;
    }

    void setResizeCallback(ResizeCallback callback);
    void setKeyInputCallback(KeyInputCallback callback);
    void setMousePositionCallback(MousePositionCallback callback);
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

    static ScanCode convertGlfwToKnotScancode(int glfwKey);
};
} // namespace knot
