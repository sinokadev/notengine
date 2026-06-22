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

    auto mesh = knot::loadModelOBJ(knot::getAssetRoot() + "assets/utah_teapot.obj");
    auto shader = scene.getResourceManager().getShader("pbrShader");
    auto material = std::make_shared<knot::PbrMaterial>(shader, 
                                                         glm::vec3(1,1,1), // albedoColor
                                                         0.2f,                           // metallicFactor
                                                         0.0f,                           // roughnessFactor
                                                         1.0f                            // aoFactor
    );

    auto cubeObject = std::make_shared<knot::Object>(mesh, material);
    scene.getObjectManager().registerObject(cubeObject);
    cubeObject->position = glm::vec3(0.0f, 0.0f, 0.0f);
    cubeObject->scale = glm::vec3(0.5, 0.5, 0.5);

    auto dirLightObj = std::make_shared<knot::DirLight>(glm::vec3(-0.2f, -1.0f, -0.3f), // direction
                                                            glm::vec3(0.1f),                // ambient
                                                            glm::vec3(1.0f, 1.0f, 1.0f),    // diffuse
                                                            glm::vec3(1.0f, 1.0f, 1.0f)     // specular
    );
    scene.getObjectManager().registerObject(dirLightObj);

    auto pointLightObj = std::make_shared<knot::PbrPointLight>(
        glm::vec3(1.0f,1.0f,1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),    // lightColor (warm orange light)
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

    engine.getWindow().setKeyInputCallback([&](knot::ScanCode code, knot::KeyState action) {
        if (action == knot::KeyState::PRESS) {
            if (code == knot::ScanCode::ESCAPE) {
                if (!stop)
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                else
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                stop = !stop;
            }

            keyStates[code] = true;
        } else if (action == knot::KeyState::RELEASE) {
            keyStates[code] = false;
        }
    });

    engine.getWindow().setMousePositionCallback([&](double xpos, double ypos) {
        if (stop)
            return;
        if (firstMouse) {
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
        }

        float xOffset = static_cast<float>(xpos) - lastX;
        float yOffset = lastY - static_cast<float>(ypos);

        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);

        scene.getMainCameraObject().rotate(xOffset, yOffset, true);
    });

    scene.setUpdateCallback([&](knot::Scene& currentScene, float deltaTime) {
        if (stop)
            return;
        totalTime += deltaTime;

        float speed = 0.5f;
        cubeObject->rotation = glm::quat(glm::vec3(sin(totalTime * 0.5f) * 0.2f, totalTime * speed, 0.0f));

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