#include <knot/renderer.h>

#include <glad/gl.h>

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace knot {

bool Renderer::init(GLADloadfunc loadProc) {
    if (!gladLoadGL(loadProc)) {
        std::cerr << "[Error] Failed to load OpenGL functions" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    initialized = true;
    return true;
}

void Renderer::beginFrame(int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

bool Renderer::renderObject(const Object& object, const Camera& camera, float aspectRatio) {
    if (!initialized) {
        return false;
    }

    if (dynamic_cast<const Camera*>(&object)) {
        return true;
    }

    if (dynamic_cast<const PongPointLight*>(&object)) {
        return true;
    }

    if (dynamic_cast<const PbrPointLight*>(&object)) {
        return true;
    }

    if (dynamic_cast<const DirLight*>(&object)) {
        return true;
    }

    if (!object.material) {
        std::cerr << "[Error] Object ID " << object.id << " has no material" << std::endl;
        return false;
    }

    const auto shader = object.material->getShader();
    if (!shader || !shader->isValid()) {
        std::cerr << "[Error] Object ID " << object.id << " has no valid shader" << std::endl;
        return false;
    }

    object.material->bind();

    shader->set("view", camera.getViewMatrix());
    shader->set("projection", glm::perspective(glm::radians(camera.fov), aspectRatio, kNearPlane, kFarPlane));
    shader->set("model", object.getWorldMatrix());
    shader->set("u_CameraPos", camera.position);

    // Set Directional Light uniforms
    if (!activeDirLights.empty()) {
        const auto* dirLight = activeDirLights.front();
        shader->set("dirLight.direction", dirLight->direction);
        shader->set("dirLight.ambient", dirLight->ambient);
        shader->set("dirLight.diffuse", dirLight->diffuse);
        shader->set("dirLight.specular", dirLight->specular);
    } else {
        shader->set("dirLight.direction", glm::vec3(0.0f, -1.0f, 0.0f));
        shader->set("dirLight.ambient", glm::vec3(0.0f));
        shader->set("dirLight.diffuse", glm::vec3(0.0f));
        shader->set("dirLight.specular", glm::vec3(0.0f));
    }

    // 2. Point Light 유니폼 설정 (PBR 자동 감쇄 및 반경 연산 반영)
    for (int i = 0; i < 2; ++i) { // POINT_LIGHT_COUNT is 1
        std::string prefix = "pointLights[" + std::to_string(i) + "].";
        
        if (i < static_cast<int>(activePbrPointLights.size())) {
            const auto* pointLight = activePbrPointLights[i];
            
            // PBR 광원 강도 기준 자동 유효 반경 계산
            float autoRadius = 5.0f * std::sqrt(pointLight->intensity);

            shader->set(prefix + "position", pointLight->position);
            shader->set(prefix + "color", pointLight->color);
            shader->set(prefix + "brightness", pointLight->intensity); // 'brightness'로 바인딩 명칭 정정
            shader->set(prefix + "radius", autoRadius);               // 자동 반경 추가

            // 구형 퐁(Phong) 셰이더와 호환성을 유지하기 위한 조명 감쇄 기본 세팅
            shader->set(prefix + "ambient", 0.05f * pointLight->color * pointLight->intensity);
            shader->set(prefix + "diffuse", pointLight->color * pointLight->intensity);
            shader->set(prefix + "specular", pointLight->color * pointLight->intensity);
            shader->set(prefix + "constant", 1.0f);
            shader->set(prefix + "linear", 0.09f);
            shader->set(prefix + "quadratic", 0.032f);

        } else if (i < static_cast<int>(activePointLights.size())) {
            const auto* pointLight = activePointLights[i];
            
            // 기존 퐁(Phong) 광원의 감쇄 계수를 기반으로 유효 반경 역산
            // (감쇄도 공식 역산: 최종 감쇄율이 대략 0.01 이하가 되는 한계 거리를 반경으로 추출)
            float intensityEstimate = glm::length(pointLight->diffuse);
            float autoRadius = 10.0f; 
            if (pointLight->quadratic > 0.0f) {
                autoRadius = std::sqrt(intensityEstimate / (pointLight->quadratic * 0.01f));
            } else if (pointLight->linear > 0.0f) {
                autoRadius = intensityEstimate / (pointLight->linear * 0.01f);
            }

            shader->set(prefix + "position", pointLight->position);
            shader->set(prefix + "ambient", pointLight->ambient);
            shader->set(prefix + "diffuse", pointLight->diffuse);
            shader->set(prefix + "specular", pointLight->specular);
            shader->set(prefix + "constant", pointLight->constant);
            shader->set(prefix + "linear", pointLight->linear);
            shader->set(prefix + "quadratic", pointLight->quadratic);

            // 신형 PBR 셰이더 데이터 파이프라인 대응용 변환 바인딩
            shader->set(prefix + "color", glm::length(pointLight->diffuse) > 0.0f ? glm::normalize(pointLight->diffuse) : glm::vec3(1.0f));
            shader->set(prefix + "brightness", intensityEstimate);
            shader->set(prefix + "radius", autoRadius);

        } else {
            // 빈 슬롯용 기본 안전장치 값들 보충
            shader->set(prefix + "position", glm::vec3(0.0f));
            shader->set(prefix + "color", glm::vec3(0.0f));
            shader->set(prefix + "brightness", 0.0f);
            shader->set(prefix + "radius", 1.0f); // 0 나누기 오류 방지용 최소 크기
            shader->set(prefix + "ambient", glm::vec3(0.0f));
            shader->set(prefix + "diffuse", glm::vec3(0.0f));
            shader->set(prefix + "specular", glm::vec3(0.0f));
            shader->set(prefix + "constant", 1.0f);
            shader->set(prefix + "linear", 0.0f);
            shader->set(prefix + "quadratic", 0.0f);
        }
    }

    if (!object.mesh || !object.mesh->isReady()) {
        std::cerr << "[Error] Object ID " << object.id << " has no valid mesh" << std::endl;
        return false;
    }

    glBindVertexArray(object.mesh->vao);
    glDrawElements(GL_TRIANGLES, object.mesh->indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return true;
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized) {
        return false;
    }

    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();

    activeDirLights.clear();
    activePointLights.clear();
    activePbrPointLights.clear();
    for (const auto& object : objectManager.getObjects()) {
        if (const auto* dl = dynamic_cast<const DirLight*>(object.get())) {
            activeDirLights.push_back(dl);
        } else if (const auto* pl = dynamic_cast<const PongPointLight*>(object.get())) {
            activePointLights.push_back(pl);
        } else if (const auto* pbrl = dynamic_cast<const PbrPointLight*>(object.get())) {
            activePbrPointLights.push_back(pbrl);
        }
    }

    for (const auto& object : objectManager.getObjects()) {
        renderObject(*object, camera, aspectRatio);
    }

    return true;
}

} // namespace knot
