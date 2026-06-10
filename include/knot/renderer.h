#pragma once
#include <glad/gl.h>
#include <knot/resources.h>

namespace knot {
    class Renderer {
    public:
        bool renderObject(const Object &object, const Camera& camera, float aspectRatio);
        bool init(GLADloadfunc loadProc);
    };
}