#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <knot/utility/unifont.h>

namespace knot::utility {
/**
 * @brief Draws ASCII text in framebuffer pixels using the debug UI renderer.
 *
 * Coordinates use the viewport's top-left as (0, 0). Each glyph occupies
 * 8x16 pixels and advances the cursor by 8 pixels. Newlines move down 16
 * pixels. Call after setting the desired viewport.
 *
 * @param x Horizontal pixel coordinate from the viewport's left edge.
 * @param y Vertical pixel coordinate from the viewport's top edge.
 * @param text ASCII text to draw. Unsupported characters render as '?'.
 * @param color RGBA text color, defaulting to opaque white.
 */
inline void drawText(int x, int y, std::string_view text, const std::array<float, 4>& color = {1.f, 1.f, 1.f, 1.f}) {
    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0 || text.empty())
        return;

    auto& s = unifont_detail::state();
    if (!s.ready)
        return;

    std::vector<unifont_detail::Vertex> vertices;
    vertices.reserve(text.size() * 6);

    int cursorX = x, cursorY = y;
    for (unsigned char ch : text) {
        if (ch == '\n') {
            cursorX = x;
            cursorY += 16;
            continue;
        }
        const int glyph = (ch >= 32 && ch <= 126) ? ch - 32 : ('?' - 32);
        const float u0 = static_cast<float>((glyph % 10) * 8) / 80.f;
        const float u1 = u0 + 8.f / 80.f;
        const float v0 = static_cast<float>((glyph / 10) * 16) / 160.f;
        const float v1 = v0 + 16.f / 160.f;

        using unifont_detail::Vertex;
        const Vertex a{static_cast<float>(cursorX), static_cast<float>(cursorY), u0, v0};
        const Vertex b{static_cast<float>(cursorX + 8), static_cast<float>(cursorY), u1, v0};
        const Vertex c{static_cast<float>(cursorX + 8), static_cast<float>(cursorY + 16), u1, v1};
        const Vertex d{static_cast<float>(cursorX), static_cast<float>(cursorY + 16), u0, v1};
        vertices.insert(vertices.end(), {a, b, c, a, c, d});
        cursorX += 8;
    }
    if (vertices.empty())
        return;

    GLint oldProgram = 0, oldVao = 0, oldArrayBuffer = 0, oldActiveTexture = 0, oldTexture = 0;
    GLint oldBlendSrcRgb = 0, oldBlendDstRgb = 0, oldBlendSrcAlpha = 0, oldBlendDstAlpha = 0;

    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldArrayBuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    const GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC_RGB, &oldBlendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &oldBlendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDstAlpha);

    glDisable(GL_DEPTH_TEST);
    // The Y-flipped screen-space vertices wind clockwise in clip space, while
    // the renderer enables CCW back-face culling for 3D geometry.
    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(s.program);
    glUniform2f(glGetUniformLocation(s.program, "viewportSize"), static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));
    glUniform4fv(glGetUniformLocation(s.program, "textColor"), 1, color.data());
    glUniform1i(glGetUniformLocation(s.program, "fontTexture"), 0);
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glBindVertexArray(s.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(unifont_detail::Vertex)), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

    glBindVertexArray(static_cast<GLuint>(oldVao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(oldArrayBuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(oldTexture));
    glActiveTexture(static_cast<GLenum>(oldActiveTexture));
    glUseProgram(static_cast<GLuint>(oldProgram));
    glBlendFuncSeparate(static_cast<GLenum>(oldBlendSrcRgb), static_cast<GLenum>(oldBlendDstRgb), static_cast<GLenum>(oldBlendSrcAlpha),
                        static_cast<GLenum>(oldBlendDstAlpha));
    if (depthWasEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (cullWasEnabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    if (!blendWasEnabled)
        glDisable(GL_BLEND);
}

namespace unifont_detail {

struct Widget {
    GLFWwindow* window = nullptr;
    int x = 0, y = 0, width = 0;
    bool slider = false;
};

inline Widget& activeWidget() {
    static Widget widget;
    return widget;
}

struct WidgetInput {
    bool active = false;
    bool clicked = false;
};

inline WidgetInput updateWidget(GLFWwindow* window, int x, int y, int width, bool slider, bool hovered) {
    auto& active = activeWidget();
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
        active = {};
        return {};
    }

    if (!active.window && hovered) {
        active = {window, x, y, width, slider};
        return {true, !slider};
    }

    const bool matches = active.window == window && active.x == x && active.y == y && active.width == width && active.slider == slider;
    return {matches, false};
}

inline bool cursorPosition(GLFWwindow* window, float& x, float& y) {
    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    int windowWidth = 0, windowHeight = 0, framebufferWidth = 0, framebufferHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    if (viewport[2] <= 0 || viewport[3] <= 0 || windowWidth <= 0 || windowHeight <= 0 || framebufferWidth <= 0 || framebufferHeight <= 0)
        return false;

    double cursorX = 0.0, cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    x = static_cast<float>(cursorX * framebufferWidth / windowWidth - viewport[0]);
    y = static_cast<float>(cursorY * framebufferHeight / windowHeight - (framebufferHeight - viewport[1] - viewport[3]));
    return true;
}

} // namespace unifont_detail

/**
 * @brief Draws a text button and returns true once when it is clicked.
 *
 * The button is rendered as [label] using the 8x16 font. Coordinates are
 * framebuffer pixels relative to the viewport, matching drawText().
 */
inline bool drawButton(GLFWwindow* window, int x, int y, std::string_view label) {
    if (!window)
        return false;

    const int width = static_cast<int>((label.size() + 2) * 8);
    float cursorX = 0.f, cursorY = 0.f;
    const bool hasCursor = unifont_detail::cursorPosition(window, cursorX, cursorY);
    const bool hovered = hasCursor && cursorX >= x && cursorX < x + width && cursorY >= y && cursorY < y + 16;
    const auto input = unifont_detail::updateWidget(window, x, y, width, false, hovered);

    const std::string text = "[" + std::string(label) + "]";
    drawText(x, y, text, hovered ? std::array<float, 4>{1.f, 0.85f, 0.35f, 1.f} : std::array<float, 4>{1.f, 1.f, 1.f, 1.f});
    return input.clicked;
}

/**
 * @brief Draws and updates an ASCII slider.
 *
 * The bar is quantized to 8-pixel font cells. Drag with the left mouse button
 * to update value, which is clamped to [minimum, maximum].
 */
inline bool drawSlider(GLFWwindow* window, int x, int y, int width, float& value, float minimum, float maximum) {
    if (!window || width < 24 || !(minimum < maximum))
        return false;

    const int cells = std::max(3, width / 8);
    const int pixelWidth = cells * 8;
    float cursorX = 0.f, cursorY = 0.f;
    const bool hasCursor = unifont_detail::cursorPosition(window, cursorX, cursorY);
    const bool hovered = hasCursor && cursorX >= x && cursorX < x + pixelWidth && cursorY >= y && cursorY < y + 16;
    const auto input = unifont_detail::updateWidget(window, x, y, pixelWidth, true, hovered);

    const float oldValue = value;
    value = std::clamp(value, minimum, maximum);
    if (input.active && hasCursor) {
        const float fraction = std::clamp((cursorX - x) / static_cast<float>(pixelWidth), 0.f, 1.f);
        value = minimum + fraction * (maximum - minimum);
    }

    const int trackCells = cells - 2;
    const int marker = static_cast<int>(std::round((value - minimum) / (maximum - minimum) * (trackCells - 1)));
    std::string text(static_cast<std::size_t>(cells), '-');
    text.front() = '[';
    text.back() = ']';
    text[static_cast<std::size_t>(marker + 1)] = '|';
    drawText(x, y, text, hovered || input.active ? std::array<float, 4>{1.f, 0.85f, 0.35f, 1.f} : std::array<float, 4>{1.f, 1.f, 1.f, 1.f});
    return value != oldValue;
}

} // namespace knot::utility
