// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <knot/engine.h>
#include <knot/scene.h>
#include <knot/resources.h>
#include <knot/utility.h>
#include <unordered_map>
#include <GLFW/glfw3.h>

// 사용자가 정의하는 커스텀 이벤트 ID 목록
enum UserEventType : uint32_t {
    PRESS_E
};

// 이벤트와 함께 전달할 데이터
struct EventData {
    int number;
};

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Knot Demo")) {
        return 1;
    }
    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    engine.setClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    knot::Scene scene;

    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto mesh = knot::loadModelOBJ(knot::getAssetRoot() + "assets/notbox.obj");
    auto shader = scene.getResourceManager().getShader("pbrShader");
    auto material = std::make_shared<knot::PbrMaterial>(shader, glm::vec3(1, 1, 1), // albedoColor
                                                        0.2f,                       // metallicFactor
                                                        0.0f,                       // roughnessFactor
                                                        1.0f                        // aoFactor
    );

    // 같은 Model을 공유하는 수많은 Object 생성
    auto model = std::make_shared<knot::Model>(mesh, material);

    constexpr int GRID_SIZE = 3000;
    constexpr float SPACING = 1.0f;

    for (int x = 0; x < GRID_SIZE; ++x) {
        for (int z = 0; z < GRID_SIZE; ++z) {
            auto object = std::make_shared<knot::Object>(model);

            object->position = glm::vec3(
                (x - GRID_SIZE / 2) * SPACING,
                0.0f,
                (z - GRID_SIZE / 2) * SPACING
            );

            object->scale = glm::vec3(0.5f);

            scene.getObjectManager().registerObject(object);
        }
    }

    auto dirLightObj = std::make_shared<knot::DirLight>(glm::vec3(-0.2f, -1.0f, -0.3f), // direction
                                                        glm::vec3(0.1f),                // ambient
                                                        glm::vec3(1.0f, 1.0f, 1.0f),    // diffuse
                                                        glm::vec3(1.0f, 1.0f, 1.0f)     // specular
    );
    scene.getObjectManager().registerObject(dirLightObj);

    auto pointLightObj = std::make_shared<knot::PbrPointLight>(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 2.0f);
    scene.getObjectManager().registerObject(pointLightObj);

    auto cameraObj = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 0.0f, 5.0f));
    scene.getObjectManager().registerObject(cameraObj);
    scene.setMainCameraObject(*cameraObj);

    std::unordered_map<knot::ScanCode, bool> keyStates;

    bool firstMouse = true;
    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;

    float totalTime = 0.0f;

    bool stop = false;

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    if (!stop)
                        glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    else
                        glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    stop = !stop;
                    event.handled = true;
                } else if (event.key == knot::ScanCode::E) {
                    engine.pushUserEvent(PRESS_E, EventData{50});
                }

                keyStates[event.key] = true;
            } else if (event.action == knot::KeyState::RELEASE) {
                keyStates[event.key] = false;
            }
        }

        if (event.type == knot::MouseMoved) {
            if (stop)
                return;

            float xOffset = static_cast<float>(event.x) - lastX;
            float yOffset = lastY - static_cast<float>(event.y);

            lastX = static_cast<float>(event.x);
            lastY = static_cast<float>(event.y);

            scene.getMainCameraObject().rotate(xOffset, yOffset, true);
            event.handled = true;
        }

        if (event.type == knot::EventType::User) {
            switch (event.userCode) {
                case PRESS_E: {
                    // std::any_cast를 사용해 데이터 안전하게 추출
                    if (event.userData.has_value()) {
                        auto data = std::any_cast<EventData>(event.userData);
                        std::cout << data.number << "\n";
                    }
                    event.handled = true;
                    break;
                }
            }
        }
    });

    scene.setUpdateCallback([&](knot::Scene& currentScene, float deltaTime) {
        static float fpsTimer = 0.0f;
        static int frameCount = 0;

        fpsTimer += deltaTime;
        frameCount++;

        if (fpsTimer >= 1.0f) {
            float fps = frameCount / fpsTimer;

            std::cout << "FPS: " << fps << "\n";

            fpsTimer = 0.0f;
            frameCount = 0;
        }
        if (stop)
            return;

        totalTime += deltaTime;

        auto& activeCamera = currentScene.getMainCameraObject();
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

    engine.setScene(scene);

    return engine.run();
}