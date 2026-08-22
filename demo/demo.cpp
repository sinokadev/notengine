// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <filesystem>
#include <knot/engine.h>
#include <knot/scene.h>
#include <knot/resources.h>
#include <knot/utility.h>
#include <unordered_map>
#include <GLFW/glfw3.h>

enum UserEventType : uint32_t {
    PRINT_FPS,
    RELOAD_SCENE
};

int main() {
    knot::Engine engine;

    if (!engine.init(1280, 720, "Knot Demo")) {
        return 1;
    }

    glfwSetInputMode(
        engine.getWindow().getHandle(),
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED
    );

    engine.setClearColor(
        0.12f,
        0.14f,
        0.18f,
        1.0f
    );

    knot::Scene scene;

    if (!scene.loadSeno(
            knot::getAssetRoot() + "assets/scene.seno")) {
        std::cerr << "[Error] Failed to load demo scene"
                  << std::endl;
        return 1;
    }

    // Scene file path and initial HDR
    const std::string scenePath = knot::getAssetRoot() + "assets/scene.seno";
    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    scene.setCamera(std::make_shared<knot::MovingCamera>(
        glm::vec3(0.0f, 2.0f, 5.0f)
    ));

    std::unordered_map<knot::ScanCode, bool> keyStates;

    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;

    bool stop = false;

    int frameCount = 0;

    engine.repeat(1000, PRINT_FPS);

    // Track last write time so we can detect file changes
    // Initialize to min so any real file timestamp will compare different
    std::filesystem::file_time_type lastWriteTime = std::filesystem::file_time_type::min();
    try {
        if (std::filesystem::exists(scenePath)) {
            lastWriteTime = std::filesystem::last_write_time(scenePath);
        }
    } catch (const std::exception&) {
        // ignore filesystem errors; lastWriteTime stays at min()
    }

    // Update callback used by the scene. Stored so it can be re-applied after reload.
    auto updateCallback = [&](knot::Scene& currentScene, float deltaTime) {
        frameCount++;

        if (stop)
            return;

        // Use the current camera instance from the scene (may change on reload)
        auto& camera = static_cast<knot::MovingCamera&>(currentScene.getCamera());

        glm::vec3 moveDir(0.0f);

        const glm::vec3 front =
            camera.getFront();

        const glm::vec3 right =
            glm::normalize(
                glm::cross(
                    front,
                    camera.worldUp
                )
            );

        if (keyStates[knot::ScanCode::W])
            moveDir += front;

        if (keyStates[knot::ScanCode::S])
            moveDir -= front;

        if (keyStates[knot::ScanCode::A])
            moveDir -= right;

        if (keyStates[knot::ScanCode::D])
            moveDir += right;

        if (glm::length(moveDir) > 0.0f) {
            camera.move(
                glm::normalize(moveDir),
                deltaTime
            );
        }
    };

    // Event callback: keyboard, mouse and user events (FPS print, reload check)
    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {

                if (event.key == knot::ScanCode::ESCAPE) {
                    if (!stop) {
                        glfwSetInputMode(
                            engine.getWindow().getHandle(),
                            GLFW_CURSOR,
                            GLFW_CURSOR_NORMAL
                        );
                    } else {
                        glfwSetInputMode(
                            engine.getWindow().getHandle(),
                            GLFW_CURSOR,
                            GLFW_CURSOR_DISABLED
                        );
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

            const float xOffset =
                static_cast<float>(event.x) - lastX;

            const float yOffset =
                lastY - static_cast<float>(event.y);

            lastX = static_cast<float>(event.x);
            lastY = static_cast<float>(event.y);

            // Query the current camera from the scene each time — reloads may replace it
            auto& cam = static_cast<knot::MovingCamera&>(scene.getCamera());
            cam.rotate(
                xOffset,
                yOffset,
                true
            );

            event.handled = true;
        }

        if (event.type == knot::EventType::User) {
            switch (event.userCode) {
            case PRINT_FPS:
                std::cout << "FPS: "
                          << frameCount
                          << "\n";

                frameCount = 0;
                event.handled = true;
                break;

            case RELOAD_SCENE: {
                // Periodic timer to check scene file modification
                try {
                    auto currentWrite = std::filesystem::last_write_time(scenePath);
                    if (currentWrite != lastWriteTime) {
                        lastWriteTime = currentWrite;
                        std::cout << "[Info] Scene file changed — reloading: " << scenePath << std::endl;

                        if (!scene.loadSeno(scenePath)) {
                            std::cerr << "[Error] Failed to reload scene\n";
                        } else {
                            // Re-apply HDR (keeps behavior consistent with initial load)
                            scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

                            // Re-apply update callback since loadSeno clears callbacks
                            scene.setUpdateCallback(updateCallback);

                            // Reset mouse center to avoid big jump after reload
                            lastX = static_cast<float>(engine.getWindow().getFramebufferWidth()) / 2.0f;
                            lastY = static_cast<float>(engine.getWindow().getFramebufferHeight()) / 2.0f;

                            std::cout << "[Info] Scene reloaded successfully\n";
                        }

                        event.handled = true;
                    }
                } catch (const std::exception& e) {
                    // ignore filesystem errors
                }

                break;
            }
            }
        }
    });

    // Start periodic reload checks (every 500 ms)
    engine.repeat(500, RELOAD_SCENE);

    // Register initial update callback
    scene.setUpdateCallback(updateCallback);

    engine.setScene(scene);

    return engine.run();
}