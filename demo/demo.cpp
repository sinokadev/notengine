#include <knot/engine.h>
#include <knot/utility.h>
#include <knot/resources.h>

#include <memory>

int main() {
    knot::Engine engine;
    engine.init(1920/2,1080/2, "Hello");

    knot::MeshGen meshgen;
    auto mesh = meshgen.createRegularPolygon(3, 1);

    auto myShader = engine.getResourceManager().getShader("my_alpha_shader");

    auto mat = std::make_shared<knot::AlphaMaterial>(myShader, glm::vec3(1.0f, 0.0f, 0.0f));

    auto& obj = engine.getObjectManager().createObject(mesh, mat);

    return engine.run();
}