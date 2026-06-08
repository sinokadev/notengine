#include <knot/engine.h>
#include <knot/utility.h>
#include <knot/resources.h>

#define def auto

def main() -> int {
    knot::Engine engine;
    engine.init(1920/2,1080/2, "Hello");

    knot::MeshGen meshgen;
    knot::Mesh mesh = meshgen.createRegularPolygon(3, 1);

    engine.getObjectManager().createObject(&mesh);

    return engine.run();
}