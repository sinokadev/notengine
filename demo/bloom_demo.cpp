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

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

class EmissiveMaterial final : public knot::Material {
public:
    EmissiveMaterial(std::shared_ptr<knot::Shader> shader, glm::vec3 color) : Material(std::move(shader)), color(color) {
    }

    void bind() override {
        if (!shader) {
            return;
        }
        shader->use();
        shader->set("u_EmissionColor", color);
    }

private:
    glm::vec3 color;
};

void addRoomPlane(knot::Scene& scene, const std::shared_ptr<knot::Mesh>& plane, const std::shared_ptr<knot::Shader>& shader,
                  const glm::vec3& position, const glm::quat& rotation, const glm::vec3& color) {
    auto material = std::make_shared<knot::PbrMaterial>(shader, color, 0.0f, 0.92f);
    auto wall = std::make_shared<knot::Object>(plane, material);
    wall->position = position;
    wall->rotation = rotation;
    scene.getObjectManager().registerObject(wall);
}

void addGlowObject(knot::Scene& scene, const std::shared_ptr<knot::Mesh>& mesh, const std::shared_ptr<knot::Shader>& shader,
                   const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color) {
    auto object = std::make_shared<knot::Object>(mesh, std::make_shared<EmissiveMaterial>(shader, color));
    object->position = position;
    object->scale = scale;
    scene.getObjectManager().registerObject(object);
}

} // namespace

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Not Engine | Bloom Demo")) {
        return 1;
    }

    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    engine.setClearColor(0.006f, 0.009f, 0.018f, 1.0f);
    knot::s.skymap = false;
    knot::s.ssao = false;
    knot::s.ssgi = false;
    knot::s.ambient = 0.12f;

    knot::Scene scene;
    scene.loadHDRMap(knot::getSourceRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto pbrShader = knot::ShaderManager::get().getShader("pbrShader");
    if (!pbrShader) {
        std::cerr << "[Error] Failed to load the PBR shader.\n";
        return 1;
    }

    auto bloomVertex = knot::getEngineRoot() + "shaders/postprocess.vert";
    auto& shaderManager = knot::ShaderManager::get();
    auto extractShader = shaderManager.createShader(
        std::make_shared<knot::ShaderSource>(bloomVertex, knot::getEngineRoot() + "shaders/bloom_extract.frag"), "demo.bloom.extract");
    auto blurShader = shaderManager.createShader(std::make_shared<knot::ShaderSource>(bloomVertex, knot::getEngineRoot() + "shaders/bloom_blur.frag"),
                                                 "demo.bloom.blur");
    auto compositeShader = shaderManager.createShader(
        std::make_shared<knot::ShaderSource>(bloomVertex, knot::getEngineRoot() + "shaders/bloom_composite.frag"), "demo.bloom.composite");
    if (!extractShader || !blurShader || !compositeShader) {
        std::cerr << "[Error] Failed to compile the bloom post-processing shaders.\n";
        return 1;
    }

    auto& renderer = knot::Renderer::get();
    if (!renderer.addPostProcessShader(extractShader,
                                       [](const knot::Shader& shader, GLuint, GLuint, const glm::ivec2&) { shader.set("u_Threshold", 0.78f); })) {
        std::cerr << "[Error] Failed to register the bloom extraction pass.\n";
        return 1;
    }

    constexpr int blurPairs = 6;
    for (int i = 0; i < blurPairs; ++i) {
        const bool horizontal = true;
        if (!renderer.addPostProcessShader(blurShader, [horizontal](const knot::Shader& shader, GLuint, GLuint,
                                                                    const glm::ivec2&) { shader.set("u_Horizontal", horizontal); }) ||
            !renderer.addPostProcessShader(
                blurShader, [](const knot::Shader& shader, GLuint, GLuint, const glm::ivec2&) { shader.set("u_Horizontal", false); })) {
            std::cerr << "[Error] Failed to register a bloom blur pass.\n";
            return 1;
        }
    }

    if (!renderer.addPostProcessShader(
            compositeShader, [](const knot::Shader& shader, GLuint, GLuint, const glm::ivec2&) { shader.set("u_BloomIntensity", 1.35f); })) {
        std::cerr << "[Error] Failed to register the bloom composite pass.\n";
        return 1;
    }

    auto plane = knot::createPlane(12.0f, 12.0f);
    addRoomPlane(scene, plane, pbrShader, {0.0f, 0.0f, 0.0f}, glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0)), {0.045f, 0.055f, 0.08f});
    addRoomPlane(scene, plane, pbrShader, {0.0f, 4.5f, -5.0f}, glm::quat(1, 0, 0, 0), {0.06f, 0.075f, 0.11f});
    addRoomPlane(scene, plane, pbrShader, {-6.0f, 2.25f, 0.0f}, glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 1, 0)), {0.09f, 0.035f, 0.055f});
    addRoomPlane(scene, plane, pbrShader, {6.0f, 2.25f, 0.0f}, glm::angleAxis(-glm::half_pi<float>(), glm::vec3(0, 1, 0)), {0.035f, 0.07f, 0.09f});

    auto sphere = knot::createSphere(32, 24);
    auto emissiveShader = shaderManager.createShader(std::make_shared<knot::ShaderSource>(knot::getEngineRoot() + "shaders/emissive_surface.vert",
                                                                                          knot::getEngineRoot() + "shaders/emissive_surface.frag"),
                                                     "demo.emissiveSurface");
    if (!emissiveShader) {
        std::cerr << "[Error] Failed to compile the emissive scene shader.\n";
        return 1;
    }
    addGlowObject(scene, sphere, emissiveShader, {0.0f, 2.0f, -1.6f}, {0.9f, 0.9f, 0.9f}, {4.0f, 1.15f, 0.2f});
    addGlowObject(scene, sphere, emissiveShader, {-2.25f, 1.55f, -2.7f}, {0.62f, 0.62f, 0.62f}, {0.16f, 2.2f, 4.2f});
    addGlowObject(scene, sphere, emissiveShader, {2.25f, 1.55f, -2.7f}, {0.62f, 0.62f, 0.62f}, {3.8f, 0.2f, 1.45f});

    auto cube = knot::createCube();
    addGlowObject(scene, cube, emissiveShader, {-2.25f, 2.6f, -2.7f}, {0.12f, 1.35f, 0.12f}, {0.16f, 2.2f, 4.2f});
    addGlowObject(scene, cube, emissiveShader, {2.25f, 2.6f, -2.7f}, {0.12f, 1.35f, 0.12f}, {3.8f, 0.2f, 1.45f});

    for (const auto& light : {
             std::make_shared<knot::PbrPointLight>(glm::vec3(0.0f, 2.0f, -1.6f), glm::vec3(1.0f, 0.32f, 0.08f), 42.0f),
             std::make_shared<knot::PbrPointLight>(glm::vec3(-2.25f, 1.55f, -2.7f), glm::vec3(0.08f, 0.48f, 1.0f), 28.0f),
             std::make_shared<knot::PbrPointLight>(glm::vec3(2.25f, 1.55f, -2.7f), glm::vec3(1.0f, 0.08f, 0.3f), 28.0f),
         }) {
        light->castsShadow = false;
        scene.getLightManager().registerLight(light);
    }

    auto sphereMaterial = std::make_shared<knot::PbrMaterial>(pbrShader, glm::vec3(0.75f, 0.78f, 0.86f), 0.72f, 0.18f);
    for (const glm::vec3 position : {glm::vec3(-1.35f, 0.82f, -0.5f), glm::vec3(1.35f, 0.82f, -0.5f)}) {
        auto reflectiveSphere = std::make_shared<knot::Object>(sphere, sphereMaterial);
        reflectiveSphere->position = position;
        reflectiveSphere->scale = glm::vec3(0.82f);
        scene.getObjectManager().registerObject(reflectiveSphere);
    }

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 2.2f, 8.2f), 50.0f);
    camera->rotate(0.0f, -5.0f);
    scene.setCamera(camera);

    std::unordered_map<knot::ScanCode, bool> keys;
    bool cursorCaptured = true;
    double lastX = static_cast<double>(engine.getWindow().getFramebufferWidth()) * 0.5;
    double lastY = static_cast<double>(engine.getWindow().getFramebufferHeight()) * 0.5;

    std::cout << "Bloom scene: warm, cyan, and magenta emitters in a dark room.\n"
                 "WASD: move | Q/E: down/up | mouse: look | ESC: toggle cursor\n";

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    cursorCaptured = !cursorCaptured;
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
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
