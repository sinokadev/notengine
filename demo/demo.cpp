#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <unordered_map>
#include <memory>

#include <knot/engine.h>
#include <knot/scene.h>
#include <knot/resources.h>
#include <knot/utility.h>

class DemoScene : public knot::Scene {
private:
    knot::Object* cubeObject = nullptr;
    float totalTime = 0.0f;

    std::unordered_map<knot::ScanCode, bool> keyStates;

    bool firstMouse = true;
    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;

public:
    DemoScene() = default;
    ~DemoScene() override = default;

    void onEnter() override {
        camera = std::make_unique<knot::MovingCamera>(glm::vec3(0.0f, 0.0f, 5.0f));

        auto mesh = knot::createCube();
        
        auto shader = knot::Engine::get().getResourceManager().getShader("alphaShader");
        auto material = std::make_shared<knot::AlphaMaterial>(shader, glm::vec3(0.2f, 0.6f, 1.0f));

        cubeObject = &objectManager.createObject(mesh, material);
        cubeObject->position = glm::vec3(0.0f, 0.0f, 0.0f);

        auto& window = knot::Engine::get().getWindow();

        window.setKeyInputCallback([this](knot::ScanCode code, knot::KeyState action) {
            if (action == knot::KeyState::PRESS) {
                keyStates[code] = true;
            } else if (action == knot::KeyState::RELEASE) {
                keyStates[code] = false;
            }
        });

        window.setMousePositionCallback([this](double xpos, double ypos) {
            if (firstMouse) {
                lastX = static_cast<float>(xpos);
                lastY = static_cast<float>(ypos);
                firstMouse = false;
            }

            float xOffset = static_cast<float>(xpos) - lastX;
            float yOffset = lastY - static_cast<float>(ypos);

            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);

            if (camera) {
                auto* movingCam = static_cast<knot::MovingCamera*>(camera.get());
                movingCam->rotate(xOffset, yOffset, true);
            }
        });
    }

    void onUpdate(float deltaTime) override {
        totalTime += deltaTime;

        float speed = 0.5f;
        if (cubeObject) {
            cubeObject->rotation = glm::quat(
                glm::vec3(sin(totalTime * 0.5f) * 0.2f, totalTime * speed, 0.0f));
        }

        if (camera) {
            auto* movingCam = static_cast<knot::MovingCamera*>(camera.get());
            glm::vec3 moveDir(0.0f);

            if (keyStates[knot::ScanCode::W])  moveDir += movingCam->front;
            if (keyStates[knot::ScanCode::S])  moveDir -= movingCam->front;
            if (keyStates[knot::ScanCode::A])  moveDir -= movingCam->right;
            if (keyStates[knot::ScanCode::D])  moveDir += movingCam->right;

            if (glm::length(moveDir) > 0.0f) {
                movingCam->move(glm::normalize(moveDir), deltaTime);
            }
        }
    }

    void onExit() override {
        auto& window = knot::Engine::get().getWindow();
        window.setKeyInputCallback(nullptr);
        window.setMousePositionCallback(nullptr);
    }
};

int main() {
    auto& engine = knot::Engine::get();
    if (!engine.init(1280, 720, "Knot Demo")) {
        return 1;
    }

    engine.changeScene<DemoScene>();

    return engine.run();
}