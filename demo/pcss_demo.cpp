// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <knot/engine.h>
#include <knot/renderer.h>
#include <knot/resources.h>
#include <knot/scene.h>
#include <knot/state.h>
#include <knot/utility.h>
#include <GLFW/glfw3.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {

void addBox(knot::Scene& scene, const std::shared_ptr<knot::Mesh>& cube, const std::shared_ptr<knot::Shader>& shader,
            const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color) {
    auto material = std::make_shared<knot::PbrMaterial>(shader, color, 0.1f, 0.6f);
    auto object = std::make_shared<knot::Object>(cube, material);
    object->position = position;
    object->scale = scale;
    scene.getObjectManager().registerObject(object);
}

void addSphere(knot::Scene& scene, const std::shared_ptr<knot::Mesh>& sphere, const std::shared_ptr<knot::Shader>& shader,
               const glm::vec3& position, float radius, const glm::vec3& color) {
    auto material = std::make_shared<knot::PbrMaterial>(shader, color, 0.2f, 0.4f);
    auto object = std::make_shared<knot::Object>(sphere, material);
    object->position = position;
    object->scale = glm::vec3(radius);
    scene.getObjectManager().registerObject(object);
}

} // namespace

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Not Engine | PCSS (Percentage-Closer Soft Shadows) Demo"))
        return 1;

    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    engine.setClearColor(0.1f, 0.12f, 0.16f, 1.0f);

    knot::s.shadow = true;
    knot::s.pcss = true;
    knot::s.pcss_light_size = 0.035f;
    knot::s.pcss_blocker_samples = 16;
    knot::s.pcss_pcf_samples = 24;
    knot::s.ambient = 0.25f;

    knot::Scene scene;
    scene.loadHDRMap(knot::getSourceRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto shader = knot::ShaderManager::get().getShader("pbrShader");
    if (!shader) {
        std::cerr << "[Error] Failed to create PBR shader\n";
        return 1;
    }

    // Ground plane
    auto planeMesh = knot::createPlane(30.0f, 30.0f);
    auto groundMat = std::make_shared<knot::PbrMaterial>(shader, glm::vec3(0.85f, 0.85f, 0.88f), 0.05f, 0.8f);
    auto ground = std::make_shared<knot::Object>(planeMesh, groundMat);
    ground->position = glm::vec3(0.0f, 0.0f, 0.0f);
    ground->rotation = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    scene.getObjectManager().registerObject(ground);

    auto cubeMesh = knot::createCube();
    auto sphereMesh = knot::createSphere(32, 24);

    // Objects demonstrating contact hardening at various heights:
    // 1. Box resting on ground (contact shadow should be razor sharp)
    addBox(scene, cubeMesh, shader, glm::vec3(-4.0f, 0.5f, 0.0f), glm::vec3(1.0f), glm::vec3(0.85f, 0.35f, 0.3f));

    // 2. Spheres at increasing heights to show penumbra broadening
    addSphere(scene, sphereMesh, shader, glm::vec3(-1.5f, 0.75f, 0.0f), 0.75f, glm::vec3(0.3f, 0.75f, 0.4f));
    addSphere(scene, sphereMesh, shader, glm::vec3(1.0f, 1.8f, 0.0f), 0.75f, glm::vec3(0.3f, 0.55f, 0.85f));
    addSphere(scene, sphereMesh, shader, glm::vec3(3.5f, 3.2f, 0.0f), 0.75f, glm::vec3(0.85f, 0.75f, 0.25f));

    // 3. Tall pillar and overhead block
    addBox(scene, cubeMesh, shader, glm::vec3(6.0f, 2.0f, -2.0f), glm::vec3(0.6f, 4.0f, 0.6f), glm::vec3(0.7f, 0.7f, 0.75f));
    addBox(scene, cubeMesh, shader, glm::vec3(6.0f, 4.3f, -2.0f), glm::vec3(1.6f, 0.6f, 1.6f), glm::vec3(0.9f, 0.4f, 0.6f));

    // Directional Light
    auto dirLight = std::make_shared<knot::DirLight>(
        glm::vec3(-0.35f, -1.0f, -0.4f),
        glm::vec3(0.05f),
        glm::vec3(1.0f, 0.98f, 0.92f),
        glm::vec3(1.0f)
    );
    scene.getLightManager().registerLight(dirLight);

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 3.5f, 8.5f), 55.0f);
    camera->rotate(0.0f, -15.0f);
    scene.setCamera(camera);

    std::unordered_map<knot::ScanCode, bool> keys;
    bool cursorCaptured = true;
    double lastX = static_cast<double>(engine.getWindow().getFramebufferWidth()) * 0.5;
    double lastY = static_cast<double>(engine.getWindow().getFramebufferHeight()) * 0.5;

    const auto updateTitle = [&] {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3);
        ss << "Not Engine | PCSS: " << (knot::s.pcss ? "ON [P]" : "OFF (PCF) [P]")
           << " | LightSize: " << knot::s.pcss_light_size << " [K/L]"
           << " | PCF Samples: " << knot::s.pcss_pcf_samples << " [N/M]";
        glfwSetWindowTitle(engine.getWindow().getHandle(), ss.str().c_str());
    };
    updateTitle();

    std::cout << "=== Not Engine PCSS Demo ===\n"
              << "P: Toggle PCSS / Classic PCF\n"
              << "K/L: Decrease / Increase PCSS light size\n"
              << "N/M: Decrease / Increase PCF sample count\n"
              << "WASD: Move | Q/E: Down/Up | Mouse: Look | ESC: Toggle cursor\n";

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    cursorCaptured = !cursorCaptured;
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                    event.handled = true;
                } else if (event.key == knot::ScanCode::P) {
                    knot::s.pcss = !knot::s.pcss;
                    updateTitle();
                    std::cout << "PCSS: " << (knot::s.pcss ? "ENABLED\n" : "DISABLED (Regular PCF)\n");
                    event.handled = true;
                } else if (event.key == knot::ScanCode::K) {
                    knot::s.pcss_light_size = std::max(0.005f, knot::s.pcss_light_size - 0.005f);
                    updateTitle();
                    std::cout << "Light size: " << knot::s.pcss_light_size << "\n";
                    event.handled = true;
                } else if (event.key == knot::ScanCode::L) {
                    knot::s.pcss_light_size = std::min(0.1f, knot::s.pcss_light_size + 0.005f);
                    updateTitle();
                    std::cout << "Light size: " << knot::s.pcss_light_size << "\n";
                    event.handled = true;
                } else if (event.key == knot::ScanCode::N) {
                    knot::s.pcss_pcf_samples = std::max(4, knot::s.pcss_pcf_samples - 4);
                    updateTitle();
                    std::cout << "PCF samples: " << knot::s.pcss_pcf_samples << "\n";
                    event.handled = true;
                } else if (event.key == knot::ScanCode::M) {
                    knot::s.pcss_pcf_samples = std::min(64, knot::s.pcss_pcf_samples + 4);
                    updateTitle();
                    std::cout << "PCF samples: " << knot::s.pcss_pcf_samples << "\n";
                    event.handled = true;
                }

                keys[event.key] = true;
            } else if (event.action == knot::KeyState::RELEASE) {
                keys[event.key] = false;
            }
        } else if (event.type == knot::MouseMoved && cursorCaptured) {
            const float xOffset = static_cast<float>(event.x - lastX);
            const float yOffset = static_cast<float>(lastY - event.y);
            lastX = event.x;
            lastY = event.y;
            camera->rotate(xOffset, yOffset);
            event.handled = true;
        }
    });

    scene.setUpdateCallback([&](knot::Scene&, float deltaTime) {
        glm::vec3 moveDirection(0.0f);
        if (keys[knot::ScanCode::W])
            moveDirection += camera->getFront();
        if (keys[knot::ScanCode::S])
            moveDirection -= camera->getFront();
        if (keys[knot::ScanCode::A])
            moveDirection -= camera->getRight();
        if (keys[knot::ScanCode::D])
            moveDirection += camera->getRight();
        if (keys[knot::ScanCode::E])
            moveDirection += camera->getUp();
        if (keys[knot::ScanCode::Q])
            moveDirection -= camera->getUp();

        if (glm::length(moveDirection) > 0.0001f) {
            camera->move(glm::normalize(moveDirection), 5.0f * deltaTime);
        }
    });

    engine.setScene(scene);
    return engine.run();
}
