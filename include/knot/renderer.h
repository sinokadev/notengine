// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <vector>
#include <knot/camera.h>
#include <knot/resources.h>
#include <knot/scene.h>

namespace knot {
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
    std::vector<const DirLight*> activeDirLights;
    std::vector<const PongPointLight*> activePointLights;
    std::vector<const PbrPointLight*> activePbrPointLights;
};
} // namespace knot
