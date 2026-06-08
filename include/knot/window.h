#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>

namespace knot {
    class Window {
        private:
        GLFWwindow* n_Id;
        bool n_Initialized = false;

        public:
        bool init(int width, int height, std::string title);

        bool active();

        void loop();

        bool isClose();
        
        void enableVsync();

        GLADloadfunc getProcAddress();

        ~Window();
    };
}