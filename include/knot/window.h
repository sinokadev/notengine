#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace knot {
    class Window {
        private:
        GLFWwindow* n_Id;
        bool n_Initialized = false;

        public:
        bool init();

        bool active();

        void loop();

        bool isClose();
        
        void enableVsync();

        GLADloadfunc getProcAddress();

        ~Window();
    };
}