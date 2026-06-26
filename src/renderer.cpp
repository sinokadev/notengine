#include <knot/renderer.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cassert>

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

    // SSBO 버퍼 생성
    glGenBuffers(1, &lightSSBO);

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
    if (!initialized) return false;

    if (dynamic_cast<const Camera*>(&object) || 
        dynamic_cast<const PbrPointLight*>(&object) || 
        dynamic_cast<const DirLight*>(&object)) {
        return true;
    }

    if (!object.material) {
        std::cerr << "[Error] Object ID " << object.id << " has no material" << std::endl;
        assert(false && "Object missing material component");
    }

    const auto shader = object.material->getShader();
    if (!shader || !shader->isValid()) {
        std::cerr << "[Error] Object ID " << object.id << " has no valid shader" << std::endl;
        assert(false && "Object material has invalid or uncompiled shader");
    }

    object.material->bind();

    shader->set("view", camera.getViewMatrix());
    shader->set("projection", glm::perspective(glm::radians(camera.fov), aspectRatio, kNearPlane, kFarPlane));
    shader->set("model", object.getWorldMatrix());
    shader->set("u_CameraPos", camera.position);

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

    // 셰이더에게 현재 활성화된 포인트 라이트 개수 제보
    shader->set("u_ActivePointLightCount", static_cast<int>(activePbrPointLights.size()));

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
    activePbrPointLights.clear();
    
    // 1. 오브젝트 배열 순회하며 라이트 컴포넌트 수집
    for (const auto& object : objectManager.getObjects()) {
        if (const auto* dl = dynamic_cast<const DirLight*>(object.get())) {
            activeDirLights.push_back(dl);
        } else if (const auto* pbrl = dynamic_cast<const PbrPointLight*>(object.get())) {
            activePbrPointLights.push_back(pbrl);
        }
    }

    // 🌟 [누락되었던 핵심 최적화 구간] SSBO 데이터 빌드 및 전송
    std::vector<GPUMovingPointLight> gpuLights;
    gpuLights.reserve(activePbrPointLights.size());

    for (const auto* light : activePbrPointLights) {
        GPUMovingPointLight gpuLight;
        gpuLight.position = glm::vec4(light->position, 1.0f);
        // rgb 영역에는 색상을, alpha(w) 영역에는 intensity(brightness)를 저장하여 16바이트 정렬 고수
        gpuLight.color = glm::vec4(light->color, light->intensity); 
        gpuLight.radius = 5.0f * std::sqrt(light->intensity);
        gpuLight.constant = 1.0f;
        gpuLight.linear = 0.09f;
        gpuLight.quadratic = 0.032f;
        gpuLights.push_back(gpuLight);
    }

    // 데이터가 존재할 때만 안전하게 전송하되, 0개일 때도 버퍼 바인딩은 안전하게 유지
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
    if (!gpuLights.empty()) {
        glBufferData(GL_SHADER_STORAGE_BUFFER, gpuLights.size() * sizeof(GPUMovingPointLight), gpuLights.data(), GL_DYNAMIC_DRAW);
    }
    // binding = 0 슬롯에 이 SSBO를 글로벌하게 고정시킵니다.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 

    // 2. 오브젝트 드로우 콜 호출 수행
    for (const auto& object : objectManager.getObjects()) {
        renderObject(*object, camera, aspectRatio);
    }

    return true;
}

} // namespace knot