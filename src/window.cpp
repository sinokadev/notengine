#include <knot/window.h>

#include <iostream>

namespace knot {

ScanCode Window::convert_glfw_to_knot_scancode(int glfw_key) {
    switch (glfw_key) {
    // --- 알파벳 ---
    case GLFW_KEY_A:
        return ScanCode::A;
    case GLFW_KEY_B:
        return ScanCode::B;
    case GLFW_KEY_C:
        return ScanCode::C;
    case GLFW_KEY_D:
        return ScanCode::D;
    case GLFW_KEY_E:
        return ScanCode::E;
    case GLFW_KEY_F:
        return ScanCode::F;
    case GLFW_KEY_G:
        return ScanCode::G;
    case GLFW_KEY_H:
        return ScanCode::H;
    case GLFW_KEY_I:
        return ScanCode::I;
    case GLFW_KEY_J:
        return ScanCode::J;
    case GLFW_KEY_K:
        return ScanCode::K;
    case GLFW_KEY_L:
        return ScanCode::L;
    case GLFW_KEY_M:
        return ScanCode::M;
    case GLFW_KEY_N:
        return ScanCode::N;
    case GLFW_KEY_O:
        return ScanCode::O;
    case GLFW_KEY_P:
        return ScanCode::P;
    case GLFW_KEY_Q:
        return ScanCode::Q;
    case GLFW_KEY_R:
        return ScanCode::R;
    case GLFW_KEY_S:
        return ScanCode::S;
    case GLFW_KEY_T:
        return ScanCode::T;
    case GLFW_KEY_U:
        return ScanCode::U;
    case GLFW_KEY_V:
        return ScanCode::V;
    case GLFW_KEY_W:
        return ScanCode::W;
    case GLFW_KEY_X:
        return ScanCode::X;
    case GLFW_KEY_Y:
        return ScanCode::Y;
    case GLFW_KEY_Z:
        return ScanCode::Z;

    // --- 숫자 ---
    case GLFW_KEY_0:
        return ScanCode::NUM_0;
    case GLFW_KEY_1:
        return ScanCode::NUM_1;
    case GLFW_KEY_2:
        return ScanCode::NUM_2;
    case GLFW_KEY_3:
        return ScanCode::NUM_3;
    case GLFW_KEY_4:
        return ScanCode::NUM_4;
    case GLFW_KEY_5:
        return ScanCode::NUM_5;
    case GLFW_KEY_6:
        return ScanCode::NUM_6;
    case GLFW_KEY_7:
        return ScanCode::NUM_7;
    case GLFW_KEY_8:
        return ScanCode::NUM_8;
    case GLFW_KEY_9:
        return ScanCode::NUM_9;

    // --- 시스템 제어 및 주요 특수키 ---
    case GLFW_KEY_ENTER:
        return ScanCode::RETURN;
    case GLFW_KEY_ESCAPE:
        return ScanCode::ESCAPE;
    case GLFW_KEY_BACKSPACE:
        return ScanCode::BACKSPACE;
    case GLFW_KEY_TAB:
        return ScanCode::TAB;
    case GLFW_KEY_SPACE:
        return ScanCode::SPACE;
    case GLFW_KEY_MINUS:
        return ScanCode::MINUS;
    case GLFW_KEY_EQUAL:
        return ScanCode::EQUALS;
    case GLFW_KEY_LEFT_BRACKET:
        return ScanCode::LEFTBRACKET;
    case GLFW_KEY_RIGHT_BRACKET:
        return ScanCode::RIGHTBRACKET;
    case GLFW_KEY_BACKSLASH:
        return ScanCode::BACKSLASH;
    case GLFW_KEY_SEMICOLON:
        return ScanCode::SEMICOLON;
    case GLFW_KEY_APOSTROPHE:
        return ScanCode::APOSTROPHE;
    case GLFW_KEY_GRAVE_ACCENT:
        return ScanCode::GRAVE;
    case GLFW_KEY_COMMA:
        return ScanCode::COMMA;
    case GLFW_KEY_PERIOD:
        return ScanCode::PERIOD;
    case GLFW_KEY_SLASH:
        return ScanCode::SLASH;
    case GLFW_KEY_CAPS_LOCK:
        return ScanCode::CAPSLOCK;

    // --- 기능키 (F1 ~ F12) ---
    case GLFW_KEY_F1:
        return ScanCode::F1;
    case GLFW_KEY_F2:
        return ScanCode::F2;
    case GLFW_KEY_F3:
        return ScanCode::F3;
    case GLFW_KEY_F4:
        return ScanCode::F4;
    case GLFW_KEY_F5:
        return ScanCode::F5;
    case GLFW_KEY_F6:
        return ScanCode::F6;
    case GLFW_KEY_F7:
        return ScanCode::F7;
    case GLFW_KEY_F8:
        return ScanCode::F8;
    case GLFW_KEY_F9:
        return ScanCode::F9;
    case GLFW_KEY_F10:
        return ScanCode::F10;
    case GLFW_KEY_F11:
        return ScanCode::F11;
    case GLFW_KEY_F12:
        return ScanCode::F12;

    // --- 에디팅 및 네비게이션 ---
    case GLFW_KEY_PRINT_SCREEN:
        return ScanCode::PRINTSCREEN;
    case GLFW_KEY_SCROLL_LOCK:
        return ScanCode::SCROLLLOCK;
    case GLFW_KEY_PAUSE:
        return ScanCode::PAUSE;
    case GLFW_KEY_INSERT:
        return ScanCode::INSERT;
    case GLFW_KEY_HOME:
        return ScanCode::HOME;
    case GLFW_KEY_PAGE_UP:
        return ScanCode::PAGEUP;
    case GLFW_KEY_DELETE:
        return ScanCode::DELETE;
    case GLFW_KEY_END:
        return ScanCode::END;
    case GLFW_KEY_PAGE_DOWN:
        return ScanCode::PAGEDOWN;
    case GLFW_KEY_RIGHT:
        return ScanCode::RIGHT;
    case GLFW_KEY_LEFT:
        return ScanCode::LEFT;
    case GLFW_KEY_DOWN:
        return ScanCode::DOWN;
    case GLFW_KEY_UP:
        return ScanCode::UP;
    case GLFW_KEY_NUM_LOCK:
        return ScanCode::NUMLOCKCLEAR;

    // --- 키패드 (Numpad) ---
    case GLFW_KEY_KP_DIVIDE:
        return ScanCode::KP_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY:
        return ScanCode::KP_MULTIPLY;
    case GLFW_KEY_KP_SUBTRACT:
        return ScanCode::KP_MINUS;
    case GLFW_KEY_KP_ADD:
        return ScanCode::KP_PLUS;
    case GLFW_KEY_KP_ENTER:
        return ScanCode::KP_ENTER;
    case GLFW_KEY_KP_1:
        return ScanCode::KP_1;
    case GLFW_KEY_KP_2:
        return ScanCode::KP_2;
    case GLFW_KEY_KP_3:
        return ScanCode::KP_3;
    case GLFW_KEY_KP_4:
        return ScanCode::KP_4;
    case GLFW_KEY_KP_5:
        return ScanCode::KP_5;
    case GLFW_KEY_KP_6:
        return ScanCode::KP_6;
    case GLFW_KEY_KP_7:
        return ScanCode::KP_7;
    case GLFW_KEY_KP_8:
        return ScanCode::KP_8;
    case GLFW_KEY_KP_9:
        return ScanCode::KP_9;
    case GLFW_KEY_KP_0:
        return ScanCode::KP_0;
    case GLFW_KEY_KP_DECIMAL:
        return ScanCode::KP_PERIOD;
    case GLFW_KEY_KP_EQUAL:
        return ScanCode::KP_EQUALS;

    // --- 모디파이어 키 ---
    case GLFW_KEY_LEFT_CONTROL:
        return ScanCode::LCTRL;
    case GLFW_KEY_LEFT_SHIFT:
        return ScanCode::LSHIFT;
    case GLFW_KEY_LEFT_ALT:
        return ScanCode::LALT;
    case GLFW_KEY_LEFT_SUPER:
        return ScanCode::LGUI;
    case GLFW_KEY_RIGHT_CONTROL:
        return ScanCode::RCTRL;
    case GLFW_KEY_RIGHT_SHIFT:
        return ScanCode::RSHIFT;
    case GLFW_KEY_RIGHT_ALT:
        return ScanCode::RALT;
    case GLFW_KEY_RIGHT_SUPER:
        return ScanCode::RGUI;

    // --- 미디어 키 ---
    case GLFW_KEY_MENU:
        return ScanCode::MENU;

    default:
        return ScanCode::UNKNOWN;
    }
}

Window::Window() = default;

Window::~Window() { shutdown(); }

bool Window::init(int width, int height, const std::string &title) {
    if (!glfwInit()) {
        std::cerr << "[Error] Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    windowHandle =
        glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!windowHandle) {
        std::cerr << "[Error] Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(windowHandle, this);
    glfwSetFramebufferSizeCallback(windowHandle, framebufferSizeCallback);
    glfwSetKeyCallback(windowHandle, key_callback);

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

void Window::enableVsync() { glfwSwapInterval(1); }

GLADloadfunc Window::getProcAddress() const {
    return reinterpret_cast<GLADloadfunc>(glfwGetProcAddress);
}

void Window::setResizeCallback(ResizeCallback callback) {
    resizeCallback = std::move(callback);
}

void Window::setKeyInputCallback(KeyInputCallback callback) {
    keyInputCallback = std::move(callback);
}

void Window::framebufferSizeCallback(GLFWwindow *window, int width,
                                     int height) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }

    self->framebufferWidth = width;
    self->framebufferHeight = height;

    if (self->resizeCallback) {
        self->resizeCallback(width, height);
    }
}

void Window::key_callback(GLFWwindow *window, int key, int scancode, int action,
                          int mods) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || !self->keyInputCallback) {
        return;
    }

    ScanCode knot_scancode = convert_glfw_to_knot_scancode(key);

    self->keyInputCallback(knot_scancode, (KeyState)action);
}
} // namespace knot
