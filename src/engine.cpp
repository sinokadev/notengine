#include <glad/gl.h>

#include <knot/window.h>
#include <knot/renderer.h>
#include <knot/manager.h>
#include <knot/engine.h>

namespace knot {
        bool Engine::init(int width, int height, std::string title) {
            this->width = width;
            this->height = height;
            this->title = title;

            if (!window.init(width, height, title)) return false;

            if (!window.active()) return false;
            if (!renderer.init(window.getProcAddress())) return false;

            window.enableVsync();

            n_Initialized = true;
            return true;
        }

        int Engine::run() {
            if (!n_Initialized) {return 1;}
            while (!window.isClose())
            {
                update();
                render();

                window.loop();
            }
            return 0;
        }

        void Engine::update() {
            float currentFrame = static_cast<float>(glfwGetTime());
            n_DeltaTime = currentFrame - n_LastFrame;
            n_LastFrame = currentFrame;

            int nowTime = glfwGetTime();
        }

        void Engine::render() {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (const auto& obj : objectManager.getObjectList()) {
                renderer.renderObject(obj);
            }
        }

        ObjectManager& Engine::getObjectManager() {
            return objectManager;
        }
}