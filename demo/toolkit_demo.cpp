// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev
// Knot Toolkit refers to a development approach that does not use Engine classes.

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <glad/gl.h>

#include <knot/toolkit.h>
#include <knot/resources.h>
#include <knot/utility.h>
#include <knot/renderer.h>
#include <knot/window.h>
#include <knot/camera.h>
#include <knot/manager.h>

int main() {
    knot::Window window;
    if (!window.init(1280, 720, "Knot Toolkit Demo")) return 1;

    if (!window.active()) {
        std::cerr << "[Error] Failed to activate window context" << std::endl;
        return 1;
    }

    knot::Renderer renderer;
    renderer.init(window.getProcAddress());

    knot::ResourceManager resourceManager;
    resourceManager.init();

    knot::ObjectManager objectManager;
    knot::MovingCamera camera;
    camera.position = glm::vec3(0.0f, 0.0f, 5.0f);

    auto mesh = knot::createCube();
    auto shader = resourceManager.getShader("alphaShader");
    auto material = std::make_shared<knot::AlphaMaterial>(shader, glm::vec3(0.2f, 0.6f, 1.0f));
    auto &cubeObject = objectManager.createObject(mesh, material);

    std::unordered_map<knot::ScanCode, bool> keyStates;
    window.setKeyInputCallback([&](knot::ScanCode code, knot::KeyState action) {
        keyStates[code] = (action == knot::KeyState::PRESS);
    });

    float lastFrame = static_cast<float>(glfwGetTime());
    float totalTime = 0.0f;

    while (!window.isClose()) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        totalTime += deltaTime;

        float speed = 0.5f;
        cubeObject.rotation = glm::quat(glm::vec3(sin(totalTime * 0.5f) * 0.2f, totalTime * speed, 0.0f));
        
        glm::vec3 moveDir(0.0f);
        if (keyStates[knot::ScanCode::W]) moveDir += camera.front;
        if (keyStates[knot::ScanCode::S]) moveDir -= camera.front;
        if (keyStates[knot::ScanCode::A]) moveDir -= camera.right;
        if (keyStates[knot::ScanCode::D]) moveDir += camera.right;
        if (glm::length(moveDir) > 0.0f) camera.move(glm::normalize(moveDir), deltaTime);

        glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        renderer.beginFrame(window.getFramebufferWidth(), window.getFramebufferHeight());
        float aspect = (float)window.getFramebufferWidth() / (float)window.getFramebufferHeight();
        
        for (const auto &object : objectManager.getObjects()) {
            renderer.renderObject(*object, camera, aspect);
        }

        window.loop();
    }

    return 0;
}