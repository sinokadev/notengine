#include <GLFW/glfw3.h>
#include <iostream>
#include <knot/window.h>

namespace knot {


bool Window::init() {
    if (!glfwInit()) {
        std::cerr << "[Error] Failed to initialize GLFW" << std::endl;
        return false;
    }
    // glfwWindowHint(GLFW_SAMPLES, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    n_Id = glfwCreateWindow(100, 100, "asdf", NULL, NULL);
    if (!n_Id) {
        std::cerr << "[Error] Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    n_Initialized = true;

    return true;
}

bool Window::active() {
    if (!n_Initialized) {std::cerr << "[Error] Window has not initialized" << std::endl; return false;}
    glfwMakeContextCurrent(n_Id);
    return true;
}

void Window::loop() {
    glfwSwapBuffers(n_Id);

    glfwPollEvents();
}

Window::~Window() {
    if (n_Id) {
        glfwDestroyWindow(n_Id);
    }
}

bool Window::isClose() {
    return glfwWindowShouldClose(n_Id);
}

}