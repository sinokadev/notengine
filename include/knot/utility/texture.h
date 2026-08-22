// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <glm/glm.hpp>
#include <string>

namespace knot {

/** @brief Creates a 1x1 RGBA texture from a linear RGB color.
 *  @return The OpenGL texture ID. */
unsigned int createSolidColorTexture(glm::vec3 color);

/** @brief Creates a mipmapped 2D texture from 8-bit pixel data.
 *  @param data Pixel data accepted by glTexImage2D.
 *  @param width Texture width in pixels.
 *  @param height Texture height in pixels.
 *  @param format OpenGL pixel format and internal format.
 *  @return The OpenGL texture ID. */
unsigned int createTexture(unsigned char* data, int width, int height, GLenum format);

/** @brief Loads an image as an RGBA 2D texture.
 *  @return The texture ID, or a magenta fallback texture if loading fails. */
unsigned int loadTextureFromFile(const std::string& path);
/** @brief Loads a floating-point HDR image as an RGB16F 2D texture.
 *  @return The texture ID, or 0 if loading fails. */
unsigned int loadHDRTexture(const std::string& path);
} // namespace knot
