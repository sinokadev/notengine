#include <knot/engine.h>
#include <knot/utility.h>
#include <knot/resources.h>

int main() {
    knot::Engine engine;
    engine.init(1920/2,1080/2, "Hello");

    knot::MeshGen meshgen;
    auto mesh = meshgen.createRegularPolygon(3, 1);

    auto& obj = engine.getObjectManager().createObject(mesh);

    return engine.run();
}