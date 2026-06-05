#include <knot/window.h>
#include <knot/renderer.h>

namespace knot {
    class Engine {
    public:
        bool init() {

        }
    private:
        Window &window;
        Renderer &renderer;

        bool n_Initialized = false;
    };
}