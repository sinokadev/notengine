#pragma once
#include <glad/gl.h>
#include <knot/resources.h>

namespace knot {
    class Renderer {
    public:
        bool renderObject(const Object &object);
        bool init(GLADloadfunc loadProc);
    };
}