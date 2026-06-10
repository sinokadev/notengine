#include <glad/gl.h>

#include <knot/window.h>
#include <knot/renderer.h>
#include <knot/manager.h>
#include <knot/camera.hpp>

namespace knot {
    class Engine {
    public:
        bool init(int width, int height, std::string title);
        int run();

        ObjectManager& getObjectManager();
        ResourceManager& getResourceManager();
    private:
        Window window;
        Renderer renderer;
        Camera camera;
        ObjectManager objectManager;
        ResourceManager resourceManager;
        int width, height;
        std::string title;

        bool n_Initialized = false;

        float n_DeltaTime = 0.0f;
        float n_LastFrame = 0.0f;

        void update();

        void render();
    };
}