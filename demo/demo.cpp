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

enum UserEventType : uint32_t { PRINT_FPS };

int main() {
    knot::Engine engine;

    if (!engine.init(1280, 720, "Knot Demo")) {
        return 1;
    }

    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    engine.setClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    knot::Scene scene;

    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto shader = scene.getResourceManager().getShader("pbrShader");

    auto model = knot::loadModelOBJWithMTL(knot::getAssetRoot() + "assets/Untitled.obj", shader);

    auto cubeObject = std::make_shared<knot::Object>(model.front());

    scene.getObjectManager().registerObject(cubeObject);

    cubeObject->position = glm::vec3(0.0f, 0.0f, 0.0f);

    cubeObject->scale = glm::vec3(0.5f);

    auto dirLight = std::make_shared<knot::DirLight>();

    dirLight->rotation = glm::quat(glm::vec3(glm::radians(45.0f), glm::radians(-20.0f), 0.0f));

    dirLight->ambient = glm::vec3(0.1f);

    dirLight->diffuse = glm::vec3(1.0f);

    dirLight->specular = glm::vec3(1.0f);

    scene.getLightManager().registerLight(dirLight);

    auto pointLight = std::make_shared<knot::PbrPointLight>(glm::vec3(1.5f, 1.5f, 2.0f), glm::vec3(1.0f), 2.0f);

    scene.getLightManager().registerLight(pointLight);

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 0.0f, 5.0f));

    scene.setCamera(camera);

    std::unordered_map<knot::ScanCode, bool> keyStates;

    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;

    float totalTime = 0.0f;

    bool stop = false;

    int frameCount = 0;

    engine.repeat(1000, PRINT_FPS);

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {

                if (event.key == knot::ScanCode::ESCAPE) {

                    if (!stop) {
                        glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    } else {
                        glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    }

                    stop = !stop;
                    event.handled = true;
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

            camera->rotate(xOffset, yOffset, true);

            event.handled = true;
        }

        if (event.type == knot::EventType::User) {
            switch (event.userCode) {
            case PRINT_FPS: {
                std::cout << "FPS: " << frameCount << "\n";

                frameCount = 0;
                event.handled = true;
                break;
            }
            }
        }
    });

    scene.setUpdateCallback([&](knot::Scene& currentScene, float deltaTime) {
        frameCount++;

        if (stop)
            return;

        totalTime += deltaTime;

        float speed = 0.5f;

        cubeObject->rotation = glm::quat(glm::vec3(sin(totalTime * 0.5f) * 0.2f, totalTime * speed, 0.0f));

        glm::vec3 moveDir(0.0f);

        const glm::vec3 front = camera->getFront();
        const glm::vec3 right = glm::normalize(glm::cross(front, camera->worldUp));

        if (keyStates[knot::ScanCode::W])
            moveDir += front;

        if (keyStates[knot::ScanCode::S])
            moveDir -= front;

        if (keyStates[knot::ScanCode::A])
            moveDir -= right;

        if (keyStates[knot::ScanCode::D])
            moveDir += right;

        if (glm::length(moveDir) > 0.0f) {
            camera->move(glm::normalize(moveDir), deltaTime);
        }
    });

    engine.setScene(scene);

    return engine.run();
}