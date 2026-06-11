#include <knot/engine.h>
#include <knot/utility.h>
#include <knot/resources.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <unordered_map> 

int main() {
    knot::Engine engine;
    if (!engine.init(1280, 720, "3D Cube Demo")) {
        return 1;
    }

    auto mesh = knot::MeshGen::createCube();
    auto shader = engine.getResourceManager().getShader("alphaShader");
    auto material = std::make_shared<knot::AlphaMaterial>(shader, glm::vec3(0.2f, 0.6f, 1.0f));
    
    auto& cubeObject = engine.getObjectManager().createObject(mesh, material);
    cubeObject.position = glm::vec3(0.0f, 0.0f, 0.0f);

    // [변경] 메인 함수에서 직접 카메라를 들고 있을 필요가 없어졌습니다!
    // 초기 위치 세팅이 필요하다면 아래처럼 엔진 내부 카메라를 꺼내서 설정해 줍니다.
    engine.getCamera().position = glm::vec3(0.0f, 0.0f, 5.0f); 

    std::unordered_map<knot::ScanCode, bool> keyStates;    

    float totalTime = 0.0f;

    engine.getWindow().setKeyInputCallback([&](knot::ScanCode code, knot::KeyState action) {
        if (action == knot::KeyState::PRESS) {
            keyStates[code] = true;
        }
        else if (action == knot::KeyState::RELEASE) {
            keyStates[code] = false;
        }
    });

    engine.setUpdateCallback([&](knot::Engine&, float deltaTime) {
        totalTime += deltaTime;

        // 1. 큐브 회전 로직
        float speed = 0.5f;
        cubeObject.rotation = glm::quat(glm::vec3(
            sin(totalTime * 0.5f) * 0.2f,
            totalTime * speed,
            0.0f
        ));

        // ---------------------------------------------------------
        // [변경] 엔진이 소유한 MovingCamera를 꺼내와서 참조로 사용합니다.
        // ---------------------------------------------------------
        auto& activeCamera = engine.getCamera();
        glm::vec3 moveDir(0.0f);

        if (keyStates[knot::ScanCode::W]) moveDir += activeCamera.front; 
        if (keyStates[knot::ScanCode::S]) moveDir -= activeCamera.front; 
        if (keyStates[knot::ScanCode::A]) moveDir -= activeCamera.right; 
        if (keyStates[knot::ScanCode::D]) moveDir += activeCamera.right; 

        if (glm::length(moveDir) > 0.0f) {
            activeCamera.move(glm::normalize(moveDir), deltaTime);
        }

        // ---------------------------------------------------------
        // [변경] 매 프레임 업데이트된 엔진 카메라의 View 행렬을 주입
        // ---------------------------------------------------------
        glm::mat4 viewMatrix = activeCamera.get_view_matrix();
        shader->use(); 
        shader->set("view", viewMatrix); 
    });

    return engine.run();
}