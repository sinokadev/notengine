// https://github.com/floooh/sokol-samples/blob/master/glfw/glfw_glue.c
/*
MIT License

Copyright (c) 2017 Andre Weissflog

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// The implementation and native context bridge live in one translation unit.
#define SOKOL_GLCORE
#define SOKOL_IMPL
#include "glfw_glue.h"
#include "sokol_log.h"

sg_environment glfw_environment(void) {
    // Query before sg_setup(), which initializes sokol's own Windows GL loader.
#if defined(_WIN32)
    typedef void(WINAPI * get_integer_fn)(unsigned int, int*);
#else
    typedef void (*get_integer_fn)(unsigned int, int*);
#endif
    const get_integer_fn get_integer = (get_integer_fn)glfwGetProcAddress("glGetIntegerv");
    int samples = 0;
    if (get_integer)
        get_integer(0x80A9 /* GL_SAMPLES */, &samples);
    return (sg_environment){
        .defaults =
            {
                .color_format = SG_PIXELFORMAT_RGBA8,
                .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
                .sample_count = samples > 0 ? samples : 1,
            },
    };
}

sg_swapchain glfw_swapchain(void) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    const sg_environment env = sg_query_desc().environment;
    return (sg_swapchain){
        .width = width,
        .height = height,
        .sample_count = env.defaults.sample_count,
        .color_format = env.defaults.color_format,
        .depth_format = env.defaults.depth_format,
    };
}
