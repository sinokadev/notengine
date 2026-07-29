// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <vector>
#include <knot/camera.h>
#include <knot/resources.h>
#include <knot/scene.h>

namespace knot {
struct GPUMovingPointLight {
    glm::vec4 position; // [x, y, z, w(Any data or Padding)]
    glm::vec4 color;    // [r, g, b, brightness]
    float radius;
    float constant;
    float linear;
    float quadratic;
};
class Renderer {
public:
    static constexpr float kNearPlane = 0.1f;
    static constexpr float kFarPlane = 100.0f;

    bool init(GLADloadfunc loadProc);
    void beginFrame(int framebufferWidth, int framebufferHeight);
    bool renderObject(const Object& object, const Camera& camera, float aspectRatio);
    void renderSkybox(unsigned int cubemapID, const Camera& camera, float aspectRatio);
    bool renderScene(Scene& scene, float aspectRatio);

    void processDirLights(const std::shared_ptr<Shader>& shader, const std::vector<const DirLight*>& dirLights);
    void processPointLights(const std::vector<const PbrPointLight*>& pointLights);
    unsigned int bakeHDRMapToCubemap(unsigned int hdrTexture2D, int size);

private:
    bool initialized = false;
    GLuint lightSSBO = 0;
    static constexpr unsigned int SKYBOX_SHADER_ID = 999999;
    std::shared_ptr<Mesh> skyboxMesh;
    std::shared_ptr<Shader> skyboxShader;
};
} // namespace knot
