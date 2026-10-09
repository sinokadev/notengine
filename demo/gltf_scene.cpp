// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <iostream>
#include <unordered_map>

#include <knot/renderer.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <knot/engine.h>
#include <knot/resources.h>
#include <knot/scene.h>
#include <knot/utility.h>

enum UserEvent : uint32_t { PRINT_FPS };

int main() {
    knot::Engine engine;

    if (!engine.init(1280, 720, "glTF Scene Demo — Sponza")) {
        return 1;
    }

    engine.setClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    knot::Scene scene;

    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    std::cout << "[Info] Loading Sponza scene...\n";

    if (!scene.loadGLTF(knot::getAssetRoot() + "assets/Sponza/glTF/Sponza.gltf")) {
        std::cerr << "[Error] Failed to load Sponza.gltf\n";
        return 1;
    }

    const auto& objects = scene.getObjectManager().getObjects();
    std::cout << "[Info] Loaded " << objects.size() << " objects\n";

    auto sunLight = std::make_shared<knot::DirLight>(glm::vec3(-0.4f, -1.0f, -0.3f), glm::vec3(0.05f), glm::vec3(1.0f, 0.97f, 0.9f), glm::vec3(1.0f));
    sunLight->intensity = 2.0f;
    scene.getLightManager().registerLight(sunLight);

    const std::vector<glm::vec3> lightPositions = {
        {0.0f, 5.0f, 0.0f}, {8.0f, 3.0f, 0.0f}, {-8.0f, 3.0f, 0.0f}, {0.0f, 3.0f, 4.0f}, {0.0f, 3.0f, -4.0f},
    };
    for (const auto& pos : lightPositions) {
        auto pl = std::make_shared<knot::PbrPointLight>(pos, glm::vec3(1.0f, 0.9f, 0.75f), 20.0f);
        scene.getLightManager().registerLight(pl);
    }

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 2.0f, 0.0f), 60.0f, 0.05f, 500.0f);
    camera->speed = 0.008f;
    scene.setCamera(camera);

    std::unordered_map<knot::ScanCode, bool> keys;
    float lastX = 640.0f;
    float lastY = 360.0f;
    bool firstMouse = true;
    bool paused = false;
    int frameCount = 0;

    engine.repeat(1000, PRINT_FPS);

    engine.setEventCallback([&](knot::Event& ev) {
        if (ev.type == knot::KeyInput) {
            if (ev.action == knot::KeyState::PRESS) {
                if (ev.key == knot::ScanCode::ESCAPE) {
                    paused = !paused;
                    firstMouse = true;
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
                    ev.handled = true;
                }

                if (ev.key == knot::ScanCode::F) {
                    const auto& objs = scene.getObjectManager().getObjects();
                    std::cout << "[Info] Object count: " << objs.size() << "\n";
                    const auto groups = scene.getObjectManager().getGroups();
                    std::cout << "[Info] Groups (" << groups.size() << "):\n";
                    for (const auto& g : groups) {
                        std::cout << "  - " << g << "\n";
                    }
                    ev.handled = true;
                }

                keys[ev.key] = true;
            } else if (ev.action == knot::KeyState::RELEASE) {
                keys[ev.key] = false;
            }
        }

        if (ev.type == knot::MouseMoved && !paused) {
            if (firstMouse) {
                lastX = static_cast<float>(ev.x);
                lastY = static_cast<float>(ev.y);
                firstMouse = false;
            }
            camera->rotate(static_cast<float>(ev.x) - lastX, lastY - static_cast<float>(ev.y));
            lastX = static_cast<float>(ev.x);
            lastY = static_cast<float>(ev.y);
            ev.handled = true;
        }

        if (ev.type == knot::EventType::User && ev.userCode == PRINT_FPS) {
            std::cout << "FPS: " << frameCount << "\n";
            frameCount = 0;
            ev.handled = true;
        }
    });

    scene.setUpdateCallback([&](knot::Scene&, float dt) {
        ++frameCount;

        if (paused)
            return;

        const float speedMul = keys[knot::ScanCode::LSHIFT] ? 4.0f : 1.0f;

        glm::vec3 moveDir(0.0f);
        if (keys[knot::ScanCode::W])
            moveDir += camera->getFront();
        if (keys[knot::ScanCode::S])
            moveDir -= camera->getFront();
        if (keys[knot::ScanCode::A])
            moveDir -= camera->getRight();
        if (keys[knot::ScanCode::D])
            moveDir += camera->getRight();
        if (keys[knot::ScanCode::E])
            moveDir += camera->getUp();
        if (keys[knot::ScanCode::Q])
            moveDir -= camera->getUp();

        if (glm::length(moveDir) > 0.0f)
            camera->move(glm::normalize(moveDir) * speedMul, dt);
    });

    engine.setScene(scene);
    return engine.run();
}
