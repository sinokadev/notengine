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

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

void addRoomPlane(knot::Scene& scene, const std::shared_ptr<knot::Mesh>& plane, const std::shared_ptr<knot::Shader>& shader,
                  const glm::vec3& position, const glm::quat& rotation, const glm::vec3& color) {
    auto material = std::make_shared<knot::PbrMaterial>(shader, color, 0.0f, 0.85f);
    auto object = std::make_shared<knot::Object>(plane, material);
    object->position = position;
    object->rotation = rotation;
    scene.getObjectManager().registerObject(object);
}

void addSphere(knot::Scene& scene, const std::shared_ptr<knot::Mesh>& sphere, const std::shared_ptr<knot::Shader>& shader, const glm::vec3& position,
               const glm::vec3& color) {
    auto material = std::make_shared<knot::PbrMaterial>(shader, color, 0.0f, 0.78f);
    auto object = std::make_shared<knot::Object>(sphere, material);
    object->position = position;
    object->scale = glm::vec3(1.44f);
    scene.getObjectManager().registerObject(object);
}

} // namespace

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Not Engine | SSGI Color Bleed Demo"))
        return 1;

    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    engine.setClearColor(0.025f, 0.03f, 0.04f, 1.0f);

    knot::s.ssgi = false;
    knot::s.ssao = false;
    knot::s.ambient = 0.2f;
    knot::s.ssgi_intensity = 0.8f;
    knot::s.ssgi_radius = 2.5f;

    knot::Scene scene;
    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto shader = scene.getShaderManager().getShader("pbrShader");
    if (!shader) {
        std::cerr << "[Error] Failed to create the PBR shader\n";
        return 1;
    }

    auto plane = knot::createPlane(8.0f, 8.0f);
    addRoomPlane(scene, plane, shader, {0.0f, 0.0f, 0.0f}, glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0)), {0.78f, 0.78f, 0.78f});
    addRoomPlane(scene, plane, shader, {0.0f, 4.0f, 0.0f}, glm::angleAxis(glm::half_pi<float>(), glm::vec3(1, 0, 0)), {0.72f, 0.72f, 0.72f});
    addRoomPlane(scene, plane, shader, {0.0f, 2.0f, -4.0f}, glm::quat(1, 0, 0, 0), {0.72f, 0.72f, 0.72f});
    addRoomPlane(scene, plane, shader, {-4.0f, 2.0f, 0.0f}, glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 1, 0)), {0.78f, 0.08f, 0.045f});
    addRoomPlane(scene, plane, shader, {4.0f, 2.0f, 0.0f}, glm::angleAxis(-glm::half_pi<float>(), glm::vec3(0, 1, 0)), {0.06f, 0.62f, 0.10f});

    auto sphere = knot::createSphere(32, 24);
    addSphere(scene, sphere, shader, {-2.85f, 0.72f, -1.1f}, {0.88f, 0.88f, 0.88f});
    addSphere(scene, sphere, shader, {2.85f, 0.72f, -1.1f}, {0.88f, 0.88f, 0.88f});
    addSphere(scene, sphere, shader, {0.0f, 0.72f, -2.0f}, {0.9f, 0.9f, 0.9f});

    auto light = std::make_shared<knot::PbrPointLight>(glm::vec3(0.0f, 3.5f, -1.0f), glm::vec3(1.0f, 0.94f, 0.82f), 24.0f);
    light->castsShadow = false;
    scene.getLightManager().registerLight(light);

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 2.0f, 7.0f), 50.0f);
    scene.setCamera(camera);

    std::unordered_map<knot::ScanCode, bool> keys;
    bool cursorCaptured = true;
    double lastX = static_cast<double>(engine.getWindow().getFramebufferWidth()) * 0.5;
    double lastY = static_cast<double>(engine.getWindow().getFramebufferHeight()) * 0.5;

    const auto updateTitle = [&] {
        const std::string title = std::string("Not Engine | SSGI Color Bleed Demo | GI ") + (knot::s.ssgi ? "ON (G)" : "OFF (G)") + " | SSAO " +
                                  (knot::s.ssao ? "ON (O)" : "OFF (O)");
        glfwSetWindowTitle(engine.getWindow().getHandle(), title.c_str());
    };
    updateTitle();

    std::cout << "Red/green walls illuminate the neutral room through screen-space diffuse bounce.\n"
                 "G: toggle SSGI | O: toggle SSAO | WASD: move | Q/E: down/up | mouse: look | ESC: cursor\n";

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    cursorCaptured = !cursorCaptured;
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                    event.handled = true;
                } else if (event.key == knot::ScanCode::G) {
                    knot::s.ssgi = !knot::s.ssgi;
                    updateTitle();
                    std::cout << "SSGI: " << (knot::s.ssgi ? "ON\n" : "OFF\n");
                    event.handled = true;
                } else if (event.key == knot::ScanCode::O) {
                    knot::s.ssao = !knot::s.ssao;
                    updateTitle();
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
            moveDirection.y += 1.0f;
        if (keys[knot::ScanCode::Q])
            moveDirection.y -= 1.0f;
        if (glm::length(moveDirection) > 0.0f)
            camera->move(glm::normalize(moveDirection), deltaTime);
    });

    engine.setScene(scene);
    return engine.run();
}
