#include <glad/gl.h>

#include <knot/window.h>
#include <knot/renderer.h>
#include <knot/manager.h>

namespace knot {
    class Engine {
    public:
        bool init();

        int run();
    private:
        Window window;
        Renderer renderer;
        ObjectManager objectManager;

        bool n_Initialized = false;

        float n_DeltaTime = 0.0f;
        float n_LastFrame = 0.0f;

        void update();

        void render();
    };
}