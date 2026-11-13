// SPDX-License-Identifier: CC0 or Unlicense
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

#include <cmath>
#include <iostream>
#include <memory>
#include <unordered_map>

namespace {

std::shared_ptr<knot::PbrMaterial> makeMaterial(const std::shared_ptr<knot::Shader>& shader, const glm::vec3& color, float metallic,
                                                float roughness) {
    return std::make_shared<knot::PbrMaterial>(shader, color, metallic, roughness);
}

void addSphere(knot::Scene& scene, const std::shared_ptr<knot::Mesh>& sphere, const std::shared_ptr<knot::Shader>& shader, const glm::vec3& position,
               float radius, const glm::vec3& color, float metallic, float roughness) {
    auto object = std::make_shared<knot::Object>(sphere, makeMaterial(shader, color, metallic, roughness));
    object->position = position;
    object->scale = glm::vec3(radius);
    scene.getObjectManager().registerObject(object);
}

} // namespace

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Not Engine | Reflection Probe Test"))
        return 1;

    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    engine.setClearColor(0.035f, 0.045f, 0.065f, 1.0f);

    knot::Scene scene;
    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto shader = scene.getShaderManager().getShader("pbrShader");
    if (!shader) {
        std::cerr << "[Error] Failed to create the PBR shader\n";
        return 1;
    }

    auto planeMesh = knot::createPlane(10.0f, 10.0f);
    auto addRoomPlane = [&](const glm::vec3& position, const glm::quat& rotation, const glm::vec3& color) {
        auto wall = std::make_shared<knot::Object>(planeMesh, makeMaterial(shader, color, 0.0f, 0.78f));
        wall->position = position;
        wall->rotation = rotation;
        scene.getObjectManager().registerObject(wall);
    };

    addRoomPlane({0.0f, 0.0f, 0.0f}, glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), {0.32f, 0.34f, 0.38f});
    addRoomPlane({0.0f, 6.0f, 0.0f}, glm::angleAxis(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)), {0.42f, 0.44f, 0.48f});
    addRoomPlane({0.0f, 3.0f, -5.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), {0.12f, 0.34f, 0.56f});
    addRoomPlane({0.0f, 3.0f, 5.0f}, glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)), {0.48f, 0.16f, 0.12f});
    addRoomPlane({-5.0f, 3.0f, 0.0f}, glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)), {0.18f, 0.42f, 0.24f});
    addRoomPlane({5.0f, 3.0f, 0.0f}, glm::angleAxis(-glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)), {0.58f, 0.42f, 0.12f});

    auto sphereMesh = knot::createSphere(32, 24);
    addSphere(scene, sphereMesh, shader, {0.0f, 1.25f, 0.0f}, 0.95f, {0.88f, 0.9f, 0.94f}, 1.0f, 0.055f);
    addSphere(scene, sphereMesh, shader, {-2.15f, 1.0f, -1.65f}, 0.58f, {0.95f, 0.58f, 0.16f}, 0.9f, 0.2f);
    addSphere(scene, sphereMesh, shader, {2.15f, 1.0f, -1.65f}, 0.58f, {0.16f, 0.56f, 0.88f}, 0.85f, 0.34f);
    addSphere(scene, sphereMesh, shader, {-2.2f, 2.9f, -3.2f}, 0.72f, {0.72f, 0.18f, 0.2f}, 0.12f, 0.26f);
    addSphere(scene, sphereMesh, shader, {2.2f, 2.9f, -3.2f}, 0.72f, {0.18f, 0.72f, 0.38f}, 0.12f, 0.26f);

    auto cubeMesh = knot::createCube();
    auto pedestalMaterial = makeMaterial(shader, {0.17f, 0.19f, 0.22f}, 0.65f, 0.3f);
    for (const glm::vec3 position : {glm::vec3(-2.15f, 0.35f, -1.65f), glm::vec3(2.15f, 0.35f, -1.65f)}) {
        auto pedestal = std::make_shared<knot::Object>(cubeMesh, pedestalMaterial);
        pedestal->position = position;
        pedestal->scale = {1.05f, 0.7f, 1.05f};
        scene.getObjectManager().registerObject(pedestal);
    }

    auto movingSphere = std::make_shared<knot::Object>(sphereMesh, makeMaterial(shader, {0.9f, 0.12f, 0.48f}, 0.82f, 0.12f));
    movingSphere->scale = glm::vec3(0.42f);
    scene.getObjectManager().registerObject(movingSphere);

    auto sun = std::make_shared<knot::DirLight>(glm::vec3(-0.4f, -1.0f, -0.3f), glm::vec3(0.06f), glm::vec3(0.8f, 0.82f, 0.9f), glm::vec3(1.0f));
    scene.getLightManager().registerLight(sun);
    scene.getLightManager().registerLight(std::make_shared<knot::PbrPointLight>(glm::vec3(0.0f, 4.7f, 0.0f), glm::vec3(1.0f, 0.82f, 0.62f), 38.0f));

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 2.25f, 3.9f));
    camera->rotate(0.0f, -5.0f);
    scene.setCamera(camera);

    if (!scene.setReflectionProbe({0.0f, 3.0f, 0.0f}, {-4.7f, 0.2f, -4.7f}, {4.7f, 5.8f, 4.7f}, 256)) {
        std::cerr << "[Error] Failed to configure the reflection probe\n";
        return 1;
    }
    if (!knot::Renderer::get().captureReflectionProbe(scene)) {
        std::cerr << "[Warning] Probe capture failed; rendering with global IBL only\n";
    }

    std::cout << "WASD: move | Q/E: down/up | mouse: look | P: toggle probe | R: recapture probe | SPACE: pause/move reflector | ESC: cursor\n"
                 "The pink metallic sphere moves through the room. Press R to update the static probe capture.\n";

    std::unordered_map<knot::ScanCode, bool> keyStates;
    bool cursorCaptured = true;
    bool animateReflector = true;
    double lastX = static_cast<double>(engine.getWindow().getFramebufferWidth()) * 0.5;
    double lastY = static_cast<double>(engine.getWindow().getFramebufferHeight()) * 0.5;
    float orbitAngle = 0.0f;

    knot::s.reflection_probe = true;

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    cursorCaptured = !cursorCaptured;
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                    event.handled = true;
                } else if (event.key == knot::ScanCode::R) {
                    const bool captured = knot::Renderer::get().captureReflectionProbe(scene);
                    std::cout << (captured ? "[Info] Reflection probe recaptured\n" : "[Error] Reflection probe capture failed\n");
                    event.handled = true;
                } else if (event.key == knot::ScanCode::P) {
                    knot::s.reflection_probe = !knot::s.reflection_probe;
                    std::cout << "[Info] Local reflection probe " << (knot::s.reflection_probe ? "ON\n" : "OFF\n");
                    event.handled = true;
                } else if (event.key == knot::ScanCode::SPACE) {
                    animateReflector = !animateReflector;
                    event.handled = true;
                }
                keyStates[event.key] = true;
            } else if (event.action == knot::KeyState::RELEASE) {
                keyStates[event.key] = false;
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
        if (keyStates[knot::ScanCode::W])
            moveDirection += camera->getFront();
        if (keyStates[knot::ScanCode::S])
            moveDirection -= camera->getFront();
        if (keyStates[knot::ScanCode::A])
            moveDirection -= camera->getRight();
        if (keyStates[knot::ScanCode::D])
            moveDirection += camera->getRight();
        if (keyStates[knot::ScanCode::E])
            moveDirection.y += 1.0f;
        if (keyStates[knot::ScanCode::Q])
            moveDirection.y -= 1.0f;
        if (glm::length(moveDirection) > 0.0f)
            camera->move(glm::normalize(moveDirection), deltaTime);

        if (animateReflector) {
            orbitAngle += deltaTime * 0.65f;
            movingSphere->position = {std::cos(orbitAngle) * 3.0f, 2.2f, std::sin(orbitAngle) * 3.0f};
        }
    });

    engine.setScene(scene);
    return engine.run();
}
