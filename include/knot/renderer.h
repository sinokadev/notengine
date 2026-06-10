#pragma once
#include <glad/gl.h>
#include <knot/resources.h>
#include <knot/camera.hpp>

namespace knot {
    class Renderer {
    public:
        bool renderObject(const Object &object, const Camera& camera, float aspectRatio);
        bool init(GLADloadfunc loadProc);
    };
}