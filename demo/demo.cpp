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

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Knot Demo")) {
        return 1;
    }
    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    engine.setClearColor(0.12f, 0.14f, 0.18f, 1.0f);

    knot::Scene scene;

    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto breadMesh = knot::loadModelOBJ(knot::getAssetRoot() + "assets/3DBread006_SQ-2K-PNG/3DBread006_SQ-2K-PNG.obj");
    auto shader = scene.getResourceManager().getShader("pbrShader");

    auto breadAo = knot::loadTextureFromFile(knot::getAssetRoot() + "assets/3DBread006_SQ-2K-PNG/3DBread006_SQ-2K-PNG_AmbientOcclusion.png");
    auto breadColor = knot::loadTextureFromFile(knot::getAssetRoot() + "assets/3DBread006_SQ-2K-PNG/3DBread006_SQ-2K-PNG_Color.png");
    auto breadNormal = knot::loadTextureFromFile(knot::getAssetRoot() + "assets/3DBread006_SQ-2K-PNG/3DBread006_SQ-2K-PNG_NormalGL.png");

    auto breadMaterial = std::make_shared<knot::PbrMaterial>(
        shader,
        glm::vec3(1,1,1),
        0.0f,
        0.5f,
        1.0f,
        breadColor,
        0,
        0,
        breadAo,
        breadNormal
    );

    auto bread = std::make_shared<knot::Object>(breadMesh, breadMaterial);
    scene.getObjectManager().registerObject(bread);
    bread->position = glm::vec3(0.0f, 0.0f, 0.0f);
    bread->scale = glm::vec3(7);

    auto pointLightObj = std::make_shared<knot::PbrPointLight>(
        glm::vec3(1.0f,1.0f,1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        2.0f
    );
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



    engine.setEventCallback([&](knot::Event &event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    if (!stop)
                        glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    else
                        glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    stop = !stop;
                    event.handled = true;
                }

                keyStates[event.key] = true;
            } else if (event.action == knot::KeyState::RELEASE) {
                keyStates[event.key] = false;
            }
        }

        if (event.type == knot::MouseMoved) {
            if (stop) return;

            float xOffset = static_cast<float>(event.x) - lastX;
            float yOffset = lastY - static_cast<float>(event.y);

            lastX = static_cast<float>(event.x);
            lastY = static_cast<float>(event.y);

            scene.getMainCameraObject().rotate(xOffset, yOffset, true);
            event.handled = true;
        }
    });

    scene.setUpdateCallback([&](knot::Scene& currentScene, float deltaTime) {
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