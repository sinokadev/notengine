#include <knot/utility/texture.h>
#include "vulkan_internal.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
namespace knot {
unsigned int createSolidColorTexture(glm::vec3 color) {
    return vk::context().upload({color.r, color.g, color.b, 1.f}, 1, 1);
}
void destroyTexture(unsigned int id) {
    auto& c = vk::context();
    auto it = c.textures.find(id);
    if (it != c.textures.end()) {
        vkDeviceWaitIdle(c.device);
        c.destroy(it->second.image);
        c.textures.erase(it);
    }
}
unsigned int createTexture(const unsigned char* data, int width, int height, PixelFormat format) {
    if (!data || width <= 0 || height <= 0)
        throw std::invalid_argument("Invalid texture data");
    int channels = int(format);
    if (channels != 3 && channels != 4)
        throw std::invalid_argument("Unsupported pixel format");
    std::vector<float> pixels(size_t(width) * height * 4);
    for (size_t i = 0; i < size_t(width) * height; ++i)
        for (int c = 0; c < 4; ++c)
            pixels[i * 4 + c] = c < channels ? data[i * channels + c] / 255.f : 1.f;
    int w = width, h = height, levels = 1;
    size_t offset = 0;
    while (w > 1 || h > 1) {
        int nw = std::max(1, w / 2), nh = std::max(1, h / 2);
        size_t next = pixels.size();
        pixels.resize(next + size_t(nw) * nh * 4);
        for (int y = 0; y < nh; ++y)
            for (int x = 0; x < nw; ++x)
                for (int c = 0; c < 4; ++c) {
                    float sum = 0;
                    for (int dy = 0; dy < 2; ++dy)
                        for (int dx = 0; dx < 2; ++dx)
                            sum += pixels[offset + (size_t(std::min(h - 1, y * 2 + dy)) * w + std::min(w - 1, x * 2 + dx)) * 4 + c];
                    pixels[next + (size_t(y) * nw + x) * 4 + c] = sum * .25f;
                }
        offset = next;
        w = nw;
        h = nh;
        ++levels;
    }
    return vk::context().upload(pixels, width, height, 1, levels);
}
unsigned int loadTextureFromFile(const std::string& path) {
    int w, h, n;
    stbi_set_flip_vertically_on_load(true);
    auto* data = stbi_load(path.c_str(), &w, &h, &n, 4);
    if (!data) {
        std::cerr << "[Error] Texture failed to load: " << path << '\n';
        return createSolidColorTexture({1, 0, 1});
    }
    try {
        auto id = createTexture(data, w, h);
        stbi_image_free(data);
        return id;
    } catch (...) {
        stbi_image_free(data);
        throw;
    }
}
unsigned int loadHDRTexture(const std::string& path) {
    int w, h, n;
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(path.c_str(), &w, &h, &n, 4);
    if (!data) {
        std::cerr << "[Error] HDR failed to load: " << path << '\n';
        return 0;
    }
    try {
        std::vector<float> pixels(data, data + size_t(w) * h * 4);
        stbi_image_free(data);
        data = nullptr;
        return vk::context().upload(pixels, w, h);
    } catch (...) {
        stbi_image_free(data);
        throw;
    }
}
} // namespace knot
