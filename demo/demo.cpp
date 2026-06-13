#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <knot/engine.h>
#include <knot/resources.h>
#include <knot/utility.h>
#include <unordered_map>

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "3D Cube Demo")) {
        return 1;
    }

    auto mesh = knot::createCube();
    auto shader = engine.getResourceManager().getShader("alphaShader");
    auto material = std::make_shared<knot::AlphaMaterial>(
        shader, glm::vec3(0.2f, 0.6f, 1.0f));

    auto &cubeObject = engine.getObjectManager().createObject(mesh, material);
    cubeObject.position = glm::vec3(0.0f, 0.0f, 0.0f);

    engine.getCamera().position = glm::vec3(0.0f, 0.0f, 5.0f);

    std::unordered_map<knot::ScanCode, bool> keyStates;

    float totalTime = 0.0f;

    engine.getWindow().setKeyInputCallback(
        [&](knot::ScanCode code, knot::KeyState action) {
            if (action == knot::KeyState::PRESS) {
                keyStates[code] = true;
            } else if (action == knot::KeyState::RELEASE) {
                keyStates[code] = false;
            }
        });

    engine.setUpdateCallback([&](knot::Engine &, float deltaTime) {
        totalTime += deltaTime;

        float speed = 0.5f;
        cubeObject.rotation = glm::quat(
            glm::vec3(sin(totalTime * 0.5f) * 0.2f, totalTime * speed, 0.0f));
        auto &activeCamera = engine.getCamera();
        glm::vec3 moveDir(0.0f);

        if (keyStates[knot::ScanCode::W])
            moveDir += activeCamera.front;
        if (keyStates[knot::ScanCode::S])
            moveDir -= activeCamera.front;
        if (keyStates[knot::ScanCode::A])
            moveDir -= activeCamera.right;
        if (keyStates[knot::ScanCode::D])
            moveDir += activeCamera.right;

        if (glm::length(moveDir) > 0.0f) {
            activeCamera.move(glm::normalize(moveDir), deltaTime);
        }
    });

    return engine.run();
}