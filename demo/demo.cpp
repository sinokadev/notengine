// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <knot/engine.h>
#include <knot/resources.h>
#include <knot/utility.h>
#include <unordered_map>

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "Knot Demo")) {
        return 1;
    }

    auto mesh = knot::createCube();
    auto shader = engine.getResourceManager().getShader("alphaShader");
    auto material = std::make_shared<knot::AlphaMaterial>(
        shader, glm::vec3(0.2f, 0.6f, 1.0f));

    auto &cubeObject = engine.getObjectManager().createObject(mesh, material);
    cubeObject.position = glm::vec3(0.0f, 0.0f, 0.0f);

    engine.getCamera().position = glm::vec3(0.0f, 0.0f, 5.0f);

    std::unordered_map<knot::ScanCode, bool> keyStates;

    // --- 마우스 회전을 위한 변수 추가 ---
    bool firstMouse = true;
    float lastX = 1280.0f / 2.0f; // 화면 중심 초기값
    float lastY = 720.0f / 2.0f;
    // ----------------------------------

    float totalTime = 0.0f;

    engine.getWindow().setKeyInputCallback(
        [&](knot::ScanCode code, knot::KeyState action) {
            if (action == knot::KeyState::PRESS) {
                keyStates[code] = true;
            } else if (action == knot::KeyState::RELEASE) {
                keyStates[code] = false;
            }
        });

    // --- 마우스 위치 콜백 함수 추가 ---
    engine.getWindow().setMousePositionCallback(
        [&](double xpos, double ypos) {
            if (firstMouse) {
                lastX = static_cast<float>(xpos);
                lastY = static_cast<float>(ypos);
                firstMouse = false;
            }

            // 마우스 이동량(Offset) 계산
            float xOffset = static_cast<float>(xpos) - lastX;
            // GLFW는 왼쪽 위가 (0,0)이고 아래로 갈수록 Y가 커지므로, 
            // 3D 카메라 피치(Pitch) 방향과 맞추기 위해 Y는 반대로 뒤집어줍니다.
            float yOffset = lastY - static_cast<float>(ypos); 

            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);

            // 카메라 회전 함수 호출
            engine.getCamera().rotate(xOffset, yOffset, true);
        });
    // ----------------------------------

    engine.setUpdateCallback([&](knot::Engine &, float deltaTime) {
        totalTime += deltaTime;

        float speed = 0.5f;
        cubeObject.rotation = glm::quat(
            glm::vec3(sin(totalTime * 0.5f) * 0.2f, totalTime * speed, 0.0f));
        auto &activeCamera = engine.getCamera();
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

    return engine.run();
}