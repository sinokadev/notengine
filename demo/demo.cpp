#include <knot/engine.h>
#include <knot/utility.h>
#include <knot/resources.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "3D Cube Demo")) {
        return 1;
    }

    auto mesh = knot::MeshGen::createCube();
    auto shader = engine.getResourceManager().getShader("alphaShader");
    auto material = std::make_shared<knot::AlphaMaterial>(shader, glm::vec3(0.2f, 0.6f, 1.0f));
    
    auto& cubeObject = engine.getObjectManager().createObject(mesh, material);
    cubeObject.position = glm::vec3(0.0f, 0.0f, 0.0f);

    float totalTime = 0.0f;

    engine.getWindow().setKeyInputCallback([](knot::ScanCode code, knot::KeyState action) {
        if (code == knot::ScanCode::A && action == knot::KeyState::PRESS) {
            std::cout << "Press A" << std::endl;
        }
    });

    engine.setUpdateCallback([&](knot::Engine&, float deltaTime) {
        totalTime += deltaTime;

        float speed = 0.5f;
        cubeObject.rotation = glm::quat(glm::vec3(
            sin(totalTime * 0.5f) * 0.2f,
            totalTime * speed,
            0.0f
        ));
    });

    return engine.run();
}