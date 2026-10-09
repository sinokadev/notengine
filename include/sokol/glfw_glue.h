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

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct glfw_desc_t {
    int width;
    int height;
    int sample_count;
    bool no_depth_buffer;
    const char* title;
    int version_major;
    int version_minor;
} glfw_desc_t;

void glfw_init(const glfw_desc_t* desc);
GLFWwindow* glfw_window(void);
int glfw_width(void);
int glfw_height(void);
sg_environment glfw_environment(void);
sg_swapchain glfw_swapchain(void);

#if defined(__cplusplus)
} // extern "C"
#endif
