#include <knot/window.h>

#include <iostream>

namespace knot {

Window::Window() = default;

Window::~Window() {
    shutdown();
}

bool Window::init(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        std::cerr << "[Error] Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    windowHandle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!windowHandle) {
        std::cerr << "[Error] Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(windowHandle, this);
    glfwSetFramebufferSizeCallback(windowHandle, framebufferSizeCallback);

    framebufferWidth = width;
    framebufferHeight = height;
    initialized = true;
    return true;
}

void Window::shutdown() {
    if (windowHandle) {
        glfwDestroyWindow(windowHandle);
        windowHandle = nullptr;
    }

    if (initialized) {
        glfwTerminate();
        initialized = false;
    }
}

bool Window::active() {
    if (!windowHandle) {
        std::cerr << "[Error] Window has not been created" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(windowHandle);
    return true;
}

void Window::loop() {
    glfwSwapBuffers(windowHandle);
    glfwPollEvents();
}

bool Window::isClose() const {
    return windowHandle ? glfwWindowShouldClose(windowHandle) : true;
}

void Window::enableVsync() {
    glfwSwapInterval(1);
}

GLADloadfunc Window::getProcAddress() const {
    return reinterpret_cast<GLADloadfunc>(glfwGetProcAddress);
}

void Window::setResizeCallback(ResizeCallback callback) {
    resizeCallback = std::move(callback);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }

    self->framebufferWidth = width;
    self->framebufferHeight = height;

    if (self->resizeCallback) {
        self->resizeCallback(width, height);
    }
}

}
