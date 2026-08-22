// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

namespace knot {
/** @brief Converts an equirectangular HDR texture into a cubemap.
 *  @param hdrTexture2D Source OpenGL 2D texture.
 *  @param size Width and height of each cubemap face.
 *  @return The generated cubemap texture ID. */
unsigned int bakeHDRMapToCubemap(unsigned int hdrTexture2D, int size);
/** @brief Convolves an environment cubemap into a diffuse irradiance map.
 *  @param envCubemapID Source environment cubemap.
 *  @param size Width and height of each generated face.
 *  @return The generated irradiance cubemap texture ID. */
unsigned int bakeCubemapToIrradianceMap(unsigned int envCubemapID, int size);
} // namespace knot
