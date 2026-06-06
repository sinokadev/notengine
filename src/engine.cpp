#include <glad/gl.h>

#include <knot/window.h>
#include <knot/renderer.h>
#include <knot/manager.h>

namespace knot {
    class Engine {
    public:
        bool init() {
            if (!window.init()) return false;
            window.active();

            glEnable(GL_MULTISAMPLE);
            glEnable(GL_DEPTH_TEST);

            glfwSwapInterval(1);

            n_Initialized = true;
            return true;
        }

        int run() {
            while (!window.isClose())
            {
                update();
                render();

                window.loop();
            }
            return true;
        }
    private:
        Window window;
        Renderer renderer;
        ObjectManager objectManager;

        bool n_Initialized = false;

        float n_DeltaTime = 0.0f;
        float n_LastFrame = 0.0f;

        void update() {
            float currentFrame = static_cast<float>(glfwGetTime());
            n_DeltaTime = currentFrame - n_LastFrame;
            n_LastFrame = currentFrame;

            int nowTime = glfwGetTime();
        }

        void render() {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (const auto& obj : objectManager.getObjectList()) {
                renderer.renderObject(obj);
            }
        }
    };
}