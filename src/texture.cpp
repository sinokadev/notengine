#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <knot/utility/texture.h>

namespace knot {

unsigned int createSolidColorTexture(glm::vec3 color) {
    unsigned char r = static_cast<unsigned char>(color.r * 255.0f);
    unsigned char g = static_cast<unsigned char>(color.g * 255.0f);
    unsigned char b = static_cast<unsigned char>(color.b * 255.0f);
    unsigned char pixelData[] = {r, g, b};

    return createTexture(pixelData, 1, 1, GL_RGB);
}

unsigned int createTexture(unsigned char *data, int width, int height,
                           GLenum format) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

unsigned int loadTextureFromFile(const std::string &path) {
    int width, height, nrComponents;
    unsigned char *data =
        stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

    if (!data) {
        std::cout << "[Error] Texture failed to load at path: " << path
                  << std::endl;
        return createSolidColorTexture(glm::vec3(1.0f, 0.0f, 1.0f));
    }

    GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
    unsigned int textureID = createTexture(data, width, height, format);

    stbi_image_free(data);
    return textureID;
}
} // namespace knot