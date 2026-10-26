#pragma once

#include <glad/gl.h>
#include <stb/stb_image.h>

#include <algorithm>
#include <string>

#include <knot/loading_image.h>

namespace knot::utility {
namespace detail {

inline GLuint compileSplashShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace detail

inline void drawLoadingSplash(int framebufferWidth, int framebufferHeight) {
    static GLuint program = 0;
    static GLuint vao = 0;
    static GLuint texture = 0;
    static bool attempted = false;
    if (!attempted) {
        attempted = true;
        constexpr const char* vertexSource = R"(#version 430 core
            layout(location = 0) in vec2 position;
            layout(location = 1) in vec2 uvIn;
            uniform vec2 scale;
            out vec2 uv;
            void main() { uv = uvIn; gl_Position = vec4(position * scale, 0.0, 1.0); }
        )";
        constexpr const char* fragmentSource = R"(#version 430 core
            in vec2 uv;
            uniform sampler2D image;
            out vec4 color;
            void main() { color = texture(image, uv); }
        )";
        const GLuint vs = detail::compileSplashShader(GL_VERTEX_SHADER, vertexSource);
        const GLuint fs = detail::compileSplashShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!vs || !fs) {
            if (vs)
                glDeleteShader(vs);
            if (fs)
                glDeleteShader(fs);
            return;
        }
        program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            glDeleteProgram(program);
            program = 0;
            return;
        }

        constexpr float vertices[] = {
            -1.f, -1.f, 0.f, 1.f, 1.f, -1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, -1.f, 1.f, 0.f, 0.f,
        };
        glGenVertexArrays(1, &vao);
        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);

        const std::string pngBase64 = knot::detail::loadingPngBase64();
        std::string png;
        png.reserve((pngBase64.size() / 4) * 3);
        unsigned int accumulator = 0;
        int bits = 0;
        for (const unsigned char c : pngBase64) {
            if (c == '=')
                break;
            const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            const char* found = std::find(alphabet, alphabet + 64, c);
            if (found == alphabet + 64)
                continue;
            accumulator = (accumulator << 6) | static_cast<unsigned int>(found - alphabet);
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                png.push_back(static_cast<char>((accumulator >> bits) & 0xff));
            }
        }

        int width = 0, height = 0, channels = 0;
        stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(png.data()), static_cast<int>(png.size()), &width, &height,
                                                &channels, STBI_rgb_alpha);
        if (!pixels)
            return;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
    }

    if (!program || !vao || !texture || framebufferWidth <= 0 || framebufferHeight <= 0)
        return;
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(program);
    const float screenAspect = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
    const float scaleX = screenAspect > 1.0f ? 1.0f / screenAspect : 1.0f;
    const float scaleY = screenAspect < 1.0f ? screenAspect : 1.0f;
    glUniform2f(glGetUniformLocation(program, "scale"), scaleX, scaleY);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program, "image"), 0);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace knot::utility
