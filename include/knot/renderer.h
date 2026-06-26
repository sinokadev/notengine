// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <vector>
#include <knot/camera.h>
#include <knot/resources.h>
#include <knot/scene.h>

namespace knot {
struct GPUMovingPointLight {
    glm::vec4 position;  // [x, y, z, w(원하는 데이터나 패딩)]
    glm::vec4 color;     // [r, g, b, brightness] -> w에 밝기를 넣어 16바이트 정렬을 맞춥니다!
    float radius;
    float constant;
    float linear;
    float quadratic;
}; // 딱 48바이트로 16바이트 배수 정렬 완성!
class Renderer {
public:
    static constexpr float kNearPlane = 0.1f;
    static constexpr float kFarPlane = 100.0f;

    bool init(GLADloadfunc loadProc);
    void beginFrame(int framebufferWidth, int framebufferHeight);
    bool renderObject(const Object& object, const Camera& camera, float aspectRatio);
    bool renderScene(Scene& scene, float aspectRatio);

private:
    bool initialized = false;
    GLuint lightSSBO = 0;
    std::vector<const DirLight*> activeDirLights;
    std::vector<const PbrPointLight*> activePbrPointLights;
};
} // namespace knot
