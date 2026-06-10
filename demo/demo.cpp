#include <knot/engine.h>
#include <knot/utility.h>
#include <knot/resources.h>

#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <memory>

int main() {
    knot::Engine engine;
    if (!engine.init(1920 / 2, 1080 / 2, "NotEngine Demo")) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }

    auto mesh = knot::MeshGen::createRegularPolygon(3, 1.0f);
    if (!mesh || !mesh->isReady()) {
        std::cerr << "Failed to create mesh" << std::endl;
        return 1;
    }

    auto shader = engine.getResourceManager().getShader("my_alpha_shader");
    if (!shader) {
        std::cerr << "Failed to load default shader" << std::endl;
        return 1;
    }

    auto material = std::make_shared<knot::AlphaMaterial>(shader, glm::vec3(0.95f, 0.35f, 0.25f));
    auto& object = engine.getObjectManager().createObject(mesh, material);

    engine.setUpdateCallback([&object](knot::Engine&, float deltaTime) {
        const glm::quat spin = glm::angleAxis(deltaTime * 0.8f, glm::vec3(0.0f, 0.0f, 1.0f));
        object.rotation = spin * object.rotation;
    });

    return engine.run();
}
