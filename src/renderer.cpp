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

    glGenBuffers(1, &lightSSBO);

    initialized = true;
    return true;
}

void Renderer::beginFrame(int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth <= 0 || framebufferHeight <= 0) return;
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

// 🌟 [변경] 오직 디렉셔널 라이트 '유니폼'만 처리하는 함수 (인자로 전달받음)
void Renderer::processDirLights(const std::shared_ptr<Shader>& shader, const std::vector<const DirLight*>& dirLights) {
    if (!shader) return;

    if (!dirLights.empty()) {
        const auto* dirLight = dirLights.front();
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
}

// 🌟 [변경] 오직 포인트 라이트 'SSBO 전송'만 처리하는 함수 (인자로 전달받음)
void Renderer::processPointLights(const std::vector<const PbrPointLight*>& pointLights) {
    std::vector<GPUMovingPointLight> gpuLights;
    gpuLights.reserve(pointLights.size());

    for (const auto* light : pointLights) {
        GPUMovingPointLight gpuLight;
        gpuLight.position = glm::vec4(light->position, 1.0f);
        gpuLight.color = glm::vec4(light->color, light->intensity); 
        gpuLight.radius = 5.0f * std::sqrt(light->intensity);
        gpuLight.constant = 1.0f;
        gpuLight.linear = 0.09f;
        gpuLight.quadratic = 0.032f;
        gpuLights.push_back(gpuLight);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
    if (!gpuLights.empty()) {
        glBufferData(GL_SHADER_STORAGE_BUFFER, gpuLights.size() * sizeof(GPUMovingPointLight), gpuLights.data(), GL_DYNAMIC_DRAW);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
}

// 🌟 [변경] 라이트 처리 로직이 완전히 거세된 순수 오브젝트 렌더 함수
bool Renderer::renderObject(const Object& object, const Camera& camera, float aspectRatio) {
    if (!initialized) return false;

    // 라이트/카메라는 그리는 대상이 아니므로 패스
    if (dynamic_cast<const Camera*>(&object) || 
        dynamic_cast<const PbrPointLight*>(&object) || 
        dynamic_cast<const DirLight*>(&object)) {
        return true;
    }

    if (!object.material) return false;
    const auto shader = object.material->getShader();
    if (!shader || !shader->isValid()) return false;

    object.material->bind();

    // 오브젝트 고유의 행렬 및 카메라 정보만 세팅
    shader->set("view", camera.getViewMatrix());
    shader->set("projection", glm::perspective(glm::radians(camera.fov), aspectRatio, kNearPlane, kFarPlane));
    shader->set("model", object.getWorldMatrix());
    shader->set("u_CameraPos", camera.position);

    // 🔴 기존에 있던 디렉셔널 라이트 유니폼 세팅 코드가 완전히 제거되었습니다!

    if (!object.mesh || !object.mesh->isReady()) return false;

    glBindVertexArray(object.mesh->vao);
    glDrawElements(GL_TRIANGLES, object.mesh->indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return true;
}

bool Renderer::renderScene(Scene& scene, float aspectRatio) {
    if (!initialized) return false;

    const auto& camera = scene.getCamera();
    auto& objectManager = scene.getObjectManager();

    // 🌟 [변경] 멤버 변수 대신 지역 변수로 라이트 포인터 수집
    std::vector<const DirLight*> localDirLights;
    std::vector<const PbrPointLight*> localPointLights;
    
    // 1. 라이트 수집
    for (const auto& object : objectManager.getObjects()) {
        if (const auto* dl = dynamic_cast<const DirLight*>(object.get())) {
            localDirLights.push_back(dl);
        } else if (const auto* pbrl = dynamic_cast<const PbrPointLight*>(object.get())) {
            localPointLights.push_back(pbrl);
        }
    }

    // 2. 포인트 라이트 글로벌 SSBO 전송 (지역 변수 벡터를 인자로 전달)
    processPointLights(localPointLights);

    // 3. 오브젝트 드로우 콜 순회
    for (const auto& object : objectManager.getObjects()) {
        // 우선 오브젝트를 바인딩하고 그리기 전에
        if (!object->material) continue;
        const auto shader = object->material->getShader();
        
        // 🌟 [핵심 변경] renderObject를 호출하기 전에, renderScene 단에서 
        // 현재 오브젝트의 셰이더를 가져와 디렉셔널 라이트 및 라이트 개수 유니폼을 구워버립니다.
        if (shader && shader->isValid()) {
            shader->use(); // 셰이더를 먼저 명시적으로 바인딩
            processDirLights(shader, localDirLights);
            shader->set("u_ActivePointLightCount", static_cast<int>(localPointLights.size()));
        }

        // 이제 완벽하게 준비된 상태에서 기하 정보만 그림
        renderObject(*object, camera, aspectRatio);
    }

    return true;
}

} // namespace knot