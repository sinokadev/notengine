#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <knot/utility.h>

#include <functional>
#include <string>

namespace knot {
class Window {
public:
    using ResizeCallback = std::function<void(int width, int height)>;
    using KeyInputCallback = std::function<void(ScanCode scancode, KeyState action)>;

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

private:
    GLFWwindow* windowHandle = nullptr;
    bool initialized = false;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    ResizeCallback resizeCallback;
    KeyInputCallback keyInputCallback;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static ScanCode convertGlfwToKnotScancode(int glfwKey);
};
} // namespace knot
