#include <knot/window.h>

#include <glad/gl.h>

#include <iostream>

namespace knot {
Window::Window() = default;

Window::~Window() {
    shutdown();
}

bool Window::init(int width, int height, const std::string& title) {
    std::cout << "[Info] Not Engine Window Init" << std::endl;

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
    glfwSetFramebufferSizeCallback(windowHandle, framebufferSizeCallback_glfw);
    glfwSetKeyCallback(windowHandle, keyCallback_glfw);
    glfwSetCursorPosCallback(windowHandle, cursorPositionCallback_glfw);
    glfwSetMouseButtonCallback(windowHandle, mouseButtonCallback_glfw);

    glfwGetFramebufferSize(windowHandle, &framebufferWidth, &framebufferHeight);

    initialized = true;
    return true;
}

void Window::shutdown() {
    if (windowHandle) {
        glfwSetWindowUserPointer(windowHandle, nullptr);
        glfwSetFramebufferSizeCallback(windowHandle, nullptr);
        glfwSetKeyCallback(windowHandle, nullptr);
        glfwSetCursorPosCallback(windowHandle, nullptr);
        glfwSetMouseButtonCallback(windowHandle, nullptr);
        glfwDestroyWindow(windowHandle);
        windowHandle = nullptr;
    }

    if (initialized) {
        glfwTerminate();
        initialized = false;
    }

    resizeCallback = nullptr;
    keyInputCallback = nullptr;
    mousePositionCallback = nullptr;
    mouseButtonCallback = nullptr;
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

void Window::disableVsync() {
    glfwSwapInterval(0);
}

GLADloadfunc Window::getProcAddress() const {
    return reinterpret_cast<GLADloadfunc>(glfwGetProcAddress);
}

void Window::setResizeCallback(ResizeCallback callback) {
    resizeCallback = std::move(callback);
}

void Window::setKeyInputCallback(KeyInputCallback callback) {
    keyInputCallback = std::move(callback);
}

void Window::setMousePositionCallback(MousePositionCallback callback) {
    mousePositionCallback = std::move(callback);
}

void Window::setMouseButtonCallback(MouseButtonCallback callback) {
    mouseButtonCallback = std::move(callback);
}

void Window::framebufferSizeCallback_glfw(GLFWwindow* window, int width, int height) {
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

void Window::keyCallback_glfw(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self || !self->keyInputCallback) {
        return;
    }

    ScanCode knotScancode = convertGlfwToKnotScancode(key);

    self->keyInputCallback(knotScancode, (KeyState)action);
}

void Window::cursorPositionCallback_glfw(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self || !self->mousePositionCallback) {
        return;
    }

    self->mousePositionCallback(xpos, ypos);
}
void Window::mouseButtonCallback_glfw(GLFWwindow* window, int button, int action, int mods) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self || !self->mouseButtonCallback) {
        return;
    }

    self->mouseButtonCallback((MouseKey)button, (KeyState)action);
}
} // namespace knot
