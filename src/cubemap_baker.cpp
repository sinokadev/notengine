#include <knot/utility/cubemap_baker.h>

#include <glad/gl.h>
#include <knot/renderer.h>

namespace knot {
unsigned int bakeHDRMapToCubemap(unsigned int hdrTexture2D, int size) {
    auto bakeSource = std::make_shared<ShaderSource>(getAssetRoot() + "assets/shaders/cubemap_bake.vert", getAssetRoot() + "assets/shaders/cubemap_bake.frag");
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
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),  glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

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
}