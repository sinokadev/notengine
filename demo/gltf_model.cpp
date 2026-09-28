// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev
//
// Demo: glTF 모델 로드 (loadModelGLTF)
//
// assets/Cube/glTF/Cube.gltf를 하나의 Model로 로드한 뒤 자동으로 회전시킵니다.
// glTF 애니메이션 트랙 재생이 아닌 데모 코드의 회전입니다.
//
// 조작법:
//   WASD          - 카메라 이동
//   마우스        - 카메라 방향
//   ESC           - 마우스 커서 토글
//   숫자 1       - Cube 표시 여부 토글

#include <iostream>
#include <unordered_map>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <knot/engine.h>
#include <knot/resources.h>
#include <knot/scene.h>
#include <knot/utility.h>

enum UserEvent : uint32_t { PRINT_FPS };

int main() {
    knot::Engine engine;

    if (!engine.init(1280, 720, "glTF Model Demo")) {
        return 1;
    }

    engine.setClearColor(0.10f, 0.12f, 0.16f, 1.0f);
    glfwSetInputMode(engine.getWindow().getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    knot::Scene scene;

    // ----------------------------------------------------------------
    // HDR 환경맵
    // ----------------------------------------------------------------
    scene.loadHDRMap(knot::getAssetRoot() + "assets/DaySkyHDRI015A_2K_HDR.hdr");

    auto pbrShader = scene.getResourceManager().getShader("pbrShader");

    // ----------------------------------------------------------------
    // Cube — loadModelGLTF로 로드 (서브메시+PBR 머티리얼 포함)
    // ----------------------------------------------------------------
    auto cubeModel = knot::loadModelGLTF(
        knot::getAssetRoot() + "assets/Cube/glTF/Cube.gltf",
        pbrShader);

    if (!cubeModel) {
        std::cerr << "[Error] Failed to load Cube.gltf\n";
        return 1;
    }

    auto cubeObj = std::make_shared<knot::Object>(cubeModel);
    cubeObj->position = glm::vec3(0.0f);
    cubeObj->scale    = glm::vec3(1.0f);
    cubeObj->setGroup("cube");
    scene.getObjectManager().registerObject(cubeObj);

    // ----------------------------------------------------------------
    // 조명
    // ----------------------------------------------------------------
    auto dirLight = std::make_shared<knot::DirLight>(
        glm::vec3(-0.3f, -1.0f, -0.5f),
        glm::vec3(0.08f),
        glm::vec3(1.0f),
        glm::vec3(1.0f));
    scene.getLightManager().registerLight(dirLight);

    auto pointLight = std::make_shared<knot::PbrPointLight>(
        glm::vec3(0.0f, 3.0f, 3.0f), glm::vec3(1.0f, 0.95f, 0.85f), 5.0f);
    scene.getLightManager().registerLight(pointLight);

    // ----------------------------------------------------------------
    // 카메라
    // ----------------------------------------------------------------
    auto camera = std::make_shared<knot::MovingCamera>(
        glm::vec3(0.0f, 0.5f, 6.0f));
    scene.setCamera(camera);

    // ----------------------------------------------------------------
    // 입력 상태
    // ----------------------------------------------------------------
    std::unordered_map<knot::ScanCode, bool> keys;
    float lastX  = 640.0f;
    float lastY  = 360.0f;
    bool firstMouse = true;
    bool  paused = false;
    bool  showCube = true;
    int   frameCount = 0;
    float totalTime  = 0.0f;

    engine.repeat(1000, PRINT_FPS);

    engine.setEventCallback([&](knot::Event& ev) {
        if (ev.type == knot::KeyInput) {
            if (ev.action == knot::KeyState::PRESS) {
                if (ev.key == knot::ScanCode::ESCAPE) {
                    paused = !paused;
                    firstMouse = true;
                    glfwSetInputMode(
                        engine.getWindow().getHandle(),
                        GLFW_CURSOR,
                        paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
                    ev.handled = true;
                }
                if (ev.key == knot::ScanCode::NUM_1) {
                    showCube = !showCube;
                    // 표시 토글: 화면 밖으로 이동
                    cubeObj->position.y = showCube ? 0.0f : -9999.0f;
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
            camera->rotate(
                static_cast<float>(ev.x) - lastX,
                lastY - static_cast<float>(ev.y));
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
        totalTime += dt;

        if (paused)
            return;

        // Cube: Y축 자동 회전
        cubeObj->rotation = glm::quat(glm::vec3(0.0f, totalTime * 0.8f, 0.0f));

        glm::vec3 moveDir(0.0f);
        if (keys[knot::ScanCode::W]) moveDir += camera->getFront();
        if (keys[knot::ScanCode::S]) moveDir -= camera->getFront();
        if (keys[knot::ScanCode::A]) moveDir -= camera->getRight();
        if (keys[knot::ScanCode::D]) moveDir += camera->getRight();

        if (glm::length(moveDir) > 0.0f)
            camera->move(glm::normalize(moveDir), dt);
    });

    engine.setScene(scene);
    return engine.run();
}
