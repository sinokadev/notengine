#pragma once
#include <glm/glm.hpp>
#include <sokol/sokol_gfx.h>
#include <string>

namespace knot {
// Texture IDs are sokol texture-view handles, never native OpenGL names.
// The returned view owns its image; release both with destroyTexture().
unsigned int createTextureView(sg_image image);
void destroyTexture(unsigned int texture);
unsigned int createSolidColorTexture(glm::vec3 color);
/** @brief Uploads RGBA8 pixels and generates the full mip chain on the CPU. */
unsigned int createTexture(const unsigned char* data, int width, int height);
/** @brief Loads RGBA8 pixels, or returns a magenta fallback on failure. */
unsigned int loadTextureFromFile(const std::string& path);
/** @brief Loads an HDR image as RGBA32F, or returns zero on failure. */
unsigned int loadHDRTexture(const std::string& path);
} // namespace knot
