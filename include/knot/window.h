#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <string>

namespace knot {
    class Window {
    public:
        using ResizeCallback = std::function<void(int width, int height)>;

        Window();
        ~Window();

        bool init(int width, int height, const std::string& title);
        void shutdown();

        bool active();
        void loop();

        bool isClose() const;
        void enableVsync();

        GLADloadfunc getProcAddress() const;

        int getFramebufferWidth() const { return framebufferWidth; }
        int getFramebufferHeight() const { return framebufferHeight; }

        void setResizeCallback(ResizeCallback callback);

    private:
        GLFWwindow* windowHandle = nullptr;
        bool initialized = false;
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        ResizeCallback resizeCallback;

        static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    };
}
