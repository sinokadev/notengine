// SPDX-License-Identifier: CC0 or Unlicense
// Copyright 2026 SinokaDev

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <knot/audio.h>
#include <knot/engine.h>
#include <knot/resources.h>
#include <knot/scene.h>
#include <knot/utility.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

struct Emitter {
    std::string clip;
    std::string group;
    glm::vec3 position;
    std::shared_ptr<knot::Object> marker;
    bool playing = false;
};

} // namespace

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Not Engine | Spatial Audio Test")) {
        return 1;
    }

    knot::Audio& audio = knot::Audio::get();
    const bool audioReady = audio.init();
    bool reverbAvailable = false;
    bool wavLoaded = false;
    bool mp3Loaded = false;
    if (audioReady) {
        const std::string audioDirectory = knot::getAssetRoot() + "assets/audio/";
        wavLoaded = audio.load(audioDirectory + "test_wav.wav", "spatialWav");
        mp3Loaded = audio.load(audioDirectory + "test_mp3.mp3", "spatialMp3");

        knot::AudioEnvironment museum;
        museum.reverbEnabled = true;
        museum.density = 0.95f;
        museum.diffusion = 0.9f;
        museum.gain = 0.32f;
        museum.gainHF = 0.72f;
        museum.decayTime = 5.0f;
        museum.decayHFRatio = 0.72f;
        museum.reflectionsGain = 0.12f;
        museum.reflectionsDelay = 0.03f;
        museum.lateReverbGain = 1.8f;
        museum.lateReverbDelay = 0.045f;
        reverbAvailable = audio.setEnvironment(museum);
        if (!reverbAvailable) {
            std::cerr << "[Warning] EFX reverb is unavailable; the room will sound dry" << std::endl;
        }
    } else {
        std::cerr << "[Warning] Spatial audio is unavailable; the scene will run silently" << std::endl;
    }

    knot::Scene scene;
    auto alphaShader = scene.getShaderManager().getShader("alphaShader");
    if (!alphaShader) {
        std::cerr << "[Error] Failed to create the alpha shader" << std::endl;
        audio.shutdown();
        return 1;
    }

    auto sphere = knot::createSphere(24, 16);
    auto makeMarker = [&](const glm::vec3& position, const glm::vec3& color, float scale) {
        auto marker = std::make_shared<knot::Object>(sphere, std::make_shared<knot::AlphaMaterial>(alphaShader, color));
        marker->position = position;
        marker->scale = glm::vec3(scale);
        scene.getObjectManager().registerObject(marker);
        return marker;
    };

    auto floor = std::make_shared<knot::Object>(knot::createPlane(24.0f, 24.0f),
                                                std::make_shared<knot::AlphaMaterial>(alphaShader, glm::vec3(0.10f, 0.12f, 0.15f)));
    floor->rotation = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    scene.getObjectManager().registerObject(floor);

    std::vector<Emitter> emitters = {
        {"spatialWav", "spatial.left", {-4.0f, 0.8f, 0.0f}, makeMarker({-4.0f, 0.8f, 0.0f}, {0.15f, 0.45f, 1.0f}, 0.7f)},
        {"spatialMp3", "spatial.right", {4.0f, 0.8f, 0.0f}, makeMarker({4.0f, 0.8f, 0.0f}, {1.0f, 0.3f, 0.1f}, 0.7f)},
        {"spatialWav", "spatial.front", {0.0f, 0.8f, -4.0f}, makeMarker({0.0f, 0.8f, -4.0f}, {0.15f, 0.9f, 0.35f}, 0.7f)},
        {"spatialMp3", "spatial.rear", {0.0f, 0.8f, 4.0f}, makeMarker({0.0f, 0.8f, 4.0f}, {0.95f, 0.75f, 0.1f}, 0.7f)},
    };

    auto camera = std::make_shared<knot::MovingCamera>(glm::vec3(0.0f, 1.7f, 9.0f));
    camera->rotate(0.0f, -100.0f);
    scene.setCamera(camera);
    auto listenerMarker = makeMarker({0.0f, 0.3f, 9.0f}, {0.9f, 0.9f, 1.0f}, 0.45f);

    const float orbitRadius = 5.0f;
    float orbitAngle = 0.0f;
    glm::vec3 orbitPosition(orbitRadius, 0.8f, 0.0f);
    auto orbitMarker = makeMarker(orbitPosition, {0.9f, 0.15f, 0.8f}, 0.75f);
    bool orbitPlaying = false;
    if (audioReady && wavLoaded) {
        orbitPlaying = audio.play3D("spatialWav", "spatial.orbit", orbitPosition, 0.4f, 1.0f, true, 1.0f, 1.0f, 24.0f);
    }

    std::cout << "WASD: move | Q/E: down/up | mouse: look | 1-4: toggle fixed emitters | 5: toggle orbiting emitter | R: toggle reverb | X: stop all "
                 "| ESC: cursor\n"
              << "Headphones recommended. Blue=left, orange=right, green=front, yellow=rear, purple=moving, white=listener.\n";
    if (!audioReady) {
        std::cout << "No OpenAL output device is available; visual markers remain active.\n";
    } else if (!wavLoaded || !mp3Loaded) {
        std::cerr << "[Warning] One or more spatial-audio clips could not be loaded" << std::endl;
    }

    std::unordered_map<knot::ScanCode, bool> keyStates;
    bool cursorCaptured = true;
    double lastX = static_cast<double>(engine.getWindow().getFramebufferWidth()) * 0.5;
    double lastY = static_cast<double>(engine.getWindow().getFramebufferHeight()) * 0.5;
    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    engine.setClearColor(0.035f, 0.045f, 0.065f, 1.0f);

    engine.setEventCallback([&](knot::Event& event) {
        if (event.type == knot::KeyInput) {
            if (event.action == knot::KeyState::PRESS) {
                if (event.key == knot::ScanCode::ESCAPE) {
                    cursorCaptured = !cursorCaptured;
                    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                    event.handled = true;
                }

                const auto keyCode = static_cast<unsigned int>(event.key);
                if (keyCode >= static_cast<unsigned int>(knot::ScanCode::NUM_1) && keyCode <= static_cast<unsigned int>(knot::ScanCode::NUM_4)) {
                    const auto index = static_cast<std::size_t>(event.key) - static_cast<std::size_t>(knot::ScanCode::NUM_1);
                    Emitter& emitter = emitters[index];
                    const bool clipLoaded = emitter.clip == "spatialWav" ? wavLoaded : mp3Loaded;
                    if (audioReady && clipLoaded) {
                        if (emitter.playing) {
                            audio.stopInGroup(emitter.clip, emitter.group);
                            emitter.playing = false;
                        } else {
                            emitter.playing = audio.play3D(emitter.clip, emitter.group, emitter.position, 0.4f, 1.0f, true, 1.0f, 1.0f, 24.0f);
                        }
                    }
                    event.handled = true;
                }

                if (event.key == knot::ScanCode::NUM_5 && audioReady && wavLoaded) {
                    if (orbitPlaying) {
                        audio.stopInGroup("spatialWav", "spatial.orbit");
                        orbitPlaying = false;
                    } else {
                        orbitPlaying = audio.play3D("spatialWav", "spatial.orbit", orbitPosition, 0.4f, 1.0f, true, 1.0f, 1.0f, 24.0f);
                    }
                    event.handled = true;
                }

                if (event.key == knot::ScanCode::R && reverbAvailable) {
                    knot::AudioEnvironment environment = audio.getEnvironment();
                    environment.reverbEnabled = !environment.reverbEnabled;
                    if (audio.setEnvironment(environment)) {
                        std::cout << "Museum reverb: " << (environment.reverbEnabled ? "ON" : "OFF") << '\n';
                    }
                    event.handled = true;
                }

                if (event.key == knot::ScanCode::X) {
                    audio.stopAll();
                    orbitPlaying = false;
                    for (auto& emitter : emitters) {
                        emitter.playing = false;
                    }
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
        if (audioReady) {
            audio.update();
        }

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
        if (glm::length(moveDirection) > 0.0f) {
            camera->move(glm::normalize(moveDirection), deltaTime);
        }

        if (audioReady) {
            audio.setListener(camera->position, camera->getFront(), camera->getUp());
        }
        listenerMarker->position = glm::vec3(camera->position.x, 0.3f, camera->position.z);

        orbitAngle += deltaTime * 0.8f;
        orbitPosition = glm::vec3(std::cos(orbitAngle) * orbitRadius, 0.8f, std::sin(orbitAngle) * orbitRadius);
        orbitMarker->position = orbitPosition;
        if (orbitPlaying) {
            audio.setSourcePosition("spatialWav", "spatial.orbit", orbitPosition);
        }
    });

    engine.setScene(scene);
    const int result = engine.run();
    audio.shutdown();
    return result;
}
