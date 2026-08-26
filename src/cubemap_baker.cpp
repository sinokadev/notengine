#include <knot/utility/cubemap_baker.h>

#include <glad/gl.h>
#include <knot/renderer.h>

namespace knot {
unsigned int bakeHDRMapToCubemap(unsigned int hdrTexture2D, int size) {
    auto bakeSource =
        std::make_shared<ShaderSource>(getAssetRoot() + "assets/shaders/cubemap_bake.vert", getAssetRoot() + "assets/shaders/cubemap_bake.frag");
    std::shared_ptr<Shader> bakeShader = std::make_shared<Shader>(bakeSource, 1999999);

    std::shared_ptr<Mesh> boxMesh = createCube();

    unsigned int cubemapID;
    glGenTextures(1, &cubemapID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // 큐브 렌더링을 위한 뷰/투영 행렬
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    // FBO 설정 및 렌더링
    unsigned int captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    // 원래 뷰포트와 깊이 테스트, 컬링 상태 백업
    GLint originalViewport[4];
    glGetIntegerv(GL_VIEWPORT, originalViewport);
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    bakeShader->use();
    bakeShader->set("equirectangularMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture2D);
    glViewport(0, 0, size, size);

    for (unsigned int i = 0; i < 6; ++i) {
        bakeShader->set("view", captureViews[i]);
        bakeShader->set("projection", captureProjection);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapID, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(boxMesh->vao);

        glDrawElements(GL_TRIANGLES, boxMesh->indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 상태 복구
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    if (cullFaceEnabled) {
        glEnable(GL_CULL_FACE);
    }
    glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);

    glDeleteFramebuffers(1, &captureFBO);
    return cubemapID;
}

unsigned int bakeCubemapToIrradianceMap(unsigned int envCubemapID, int size = 32) {
    // 1. 컨벌루션 쉐이더 및 큐브 메쉬 로드
    auto irradSource = std::make_shared<ShaderSource>(getAssetRoot() + "assets/shaders/irradiance_convolution.vert",
                                                      getAssetRoot() + "assets/shaders/irradiance_convolution.frag");
    std::shared_ptr<Shader> irradShader = std::make_shared<Shader>(irradSource, 2000000); // 셰이더 ID 겹치지 않게 예시 설정

    std::shared_ptr<Mesh> boxMesh = createCube();

    // 2. 결과 저장용 Irradiance 큐브맵 생성
    unsigned int irradianceMapID;
    glGenTextures(1, &irradianceMapID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapID);
    for (unsigned int i = 0; i < 6; ++i) {
        // Irradiance 맵은 해상도가 낮아도(32x32) 충분하므로 사이즈를 작게 인자로 받습니다.
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // 3. 뷰 / 투영 행렬 설정
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    // 4. FBO 설정 및 기존 그래픽스 상태 백업
    unsigned int captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    GLint originalViewport[4];
    glGetIntegerv(GL_VIEWPORT, originalViewport);
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // 5. 텍스처 바인딩 (이 부분이 변경됨: 2D가 아니라 큐브맵을 넘겨줌)
    irradShader->use();
    irradShader->set("environmentMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemapID); // 입력 받은 원본 환경 큐브맵 바인딩

    glViewport(0, 0, size, size); // 보통 32x32 크기로 지정됨

    // 6. 6개 면 렌더링 진행
    for (unsigned int i = 0; i < 6; ++i) {
        irradShader->set("view", captureViews[i]);
        irradShader->set("projection", captureProjection);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMapID, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(boxMesh->vao);
        glDrawElements(GL_TRIANGLES, boxMesh->indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 7. 이전 상태 복구
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    if (cullFaceEnabled)
        glEnable(GL_CULL_FACE);
    glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);

    glDeleteFramebuffers(1, &captureFBO);

    return irradianceMapID; // 최종 구워진 Diffuse IBL용 큐브맵 ID 반환
}

unsigned int bakeCubemapToPrefilterMap(unsigned int envCubemapID, int size) {
    // 1. Prefilter 셰이더 및 큐브 메쉬 로드
    auto prefilterSource = std::make_shared<ShaderSource>(getAssetRoot() + "assets/shaders/prefilter.vert",
                                                         getAssetRoot() + "assets/shaders/prefilter.frag");
    std::shared_ptr<Shader> prefilterShader = std::make_shared<Shader>(prefilterSource, 2000001);

    std::shared_ptr<Mesh> boxMesh = createCube();

    // 2. 결과 저장용 Prefilter 큐브맵 생성 (Mipmap 필수)
    unsigned int prefilterMapID;
    glGenTextures(1, &prefilterMapID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMapID);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    // Mipmap Trilinear 필터링 설정
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Mipmap 메모리 할당
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // 3. 뷰 / 투영 행렬 설정
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    // 4. FBO 설정 및 상태 백업
    unsigned int captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    GLint originalViewport[4];
    glGetIntegerv(GL_VIEWPORT, originalViewport);
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // 5. 텍스처 바인딩
    prefilterShader->use();
    prefilterShader->set("environmentMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemapID);

    // 6. Mipmap Level 및 Roughness별 렌더링 (최대 5레벨: 0~4)
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        // Mipmap 레벨에 따른 해상도 축소 (예: 128 -> 64 -> 32 -> 16 -> 8)
        unsigned int mipWidth = static_cast<unsigned int>(size * std::pow(0.5f, mip));
        unsigned int mipHeight = static_cast<unsigned int>(size * std::pow(0.5f, mip));
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
        prefilterShader->set("roughness", roughness);

        for (unsigned int i = 0; i < 6; ++i) {
            prefilterShader->set("view", captureViews[i]);
            prefilterShader->set("projection", captureProjection);
            
            // 핵심: Mipmap 레벨(mip)을 지정하여 FBO에 결합
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMapID, mip);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindVertexArray(boxMesh->vao);
            glDrawElements(GL_TRIANGLES, boxMesh->indexCount, GL_UNSIGNED_INT, nullptr);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 7. 이전 상태 복구
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    if (cullFaceEnabled)
        glEnable(GL_CULL_FACE);
    glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);

    glDeleteFramebuffers(1, &captureFBO);

    return prefilterMapID;
}

} // namespace knot