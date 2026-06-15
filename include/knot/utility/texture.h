#pragma once

#include <glm/glm.hpp>
#include <string>

namespace knot {

unsigned int createSolidColorTexture(glm::vec3 color);

unsigned int createTexture(unsigned char* data, int width, int height, GLenum format);

unsigned int loadTextureFromFile(const std::string& path);
} // namespace knot