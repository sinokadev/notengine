#pragma once
#include <glm/glm.hpp>
#include <string>
namespace knot {
enum class PixelFormat { RGB = 3, RGBA = 4 };
/** @brief Creates a texture from linear RGB. Handles belong to the current Renderer lifetime. */
unsigned int createSolidColorTexture(glm::vec3 color);
/** @brief Uploads tightly packed linear 8-bit pixels, including a mip chain. */
unsigned int createTexture(const unsigned char* data, int width, int height, PixelFormat format = PixelFormat::RGBA);
/** @brief Releases a texture. Safe after renderer shutdown and for handle zero. */
void destroyTexture(unsigned int texture);
unsigned int loadTextureFromFile(const std::string& path);
unsigned int loadHDRTexture(const std::string& path);
} // namespace knot
