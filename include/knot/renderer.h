#pragma once

#include <knot/camera.h>
#include <knot/resources.h>

namespace knot {
class Renderer {
public:
    static constexpr float kNearPlane = 0.1f;
    static constexpr float kFarPlane = 100.0f;

    bool init(GLADloadfunc loadProc);
    void beginFrame(int framebufferWidth, int framebufferHeight);
    bool renderObject(const Object &object, const Camera &camera,
                      float aspectRatio);

private:
    bool initialized = false;
};
} // namespace knot
