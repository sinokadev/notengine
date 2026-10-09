#include <knot/utility/texture.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace knot {
unsigned int createTextureView(sg_image image) {
    if (!sg_isvalid())
        return 0;
    if (sg_query_image_state(image) != SG_RESOURCESTATE_VALID) {
        sg_destroy_image(image);
        return 0;
    }
    sg_view_desc desc{};
    desc.texture.image = image;
    auto view = sg_make_view(desc);
    if (sg_query_view_state(view) != SG_RESOURCESTATE_VALID) {
        sg_destroy_view(view);
        sg_destroy_image(image);
        return 0;
    }
    return view.id;
}

void destroyTexture(unsigned int texture) {
    if (!sg_isvalid() || !texture)
        return;
    const sg_view view{texture};
    const auto image = sg_query_view_image(view);
    sg_destroy_view(view);
    sg_destroy_image(image);
}

unsigned int createSolidColorTexture(glm::vec3 color) {
    color = glm::clamp(color, 0.0f, 1.0f);
    const unsigned char pixel[] = {static_cast<unsigned char>(color.r * 255), static_cast<unsigned char>(color.g * 255),
                                   static_cast<unsigned char>(color.b * 255), 255};
    return createTexture(pixel, 1, 1);
}

unsigned int createTexture(const unsigned char* data, int width, int height) {
    if (!sg_isvalid() || !data || width <= 0 || height <= 0)
        return 0;
    sg_image_desc desc{};
    desc.width = width;
    desc.height = height;
    desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    desc.num_mipmaps = 1;
    desc.data.mip_levels[0] = {data, static_cast<size_t>(width) * height * 4};
    std::array<std::vector<unsigned char>, SG_MAX_MIPMAPS> levels;
    while ((width > 1 || height > 1) && desc.num_mipmaps < SG_MAX_MIPMAPS) {
        const int w = std::max(1, width / 2), h = std::max(1, height / 2);
        auto& pixels = levels[desc.num_mipmaps];
        pixels.resize(static_cast<size_t>(w) * h * 4);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Area averaging also includes the last row/column of odd-sized images.
                const int x0 = x * width / w, x1 = (x + 1) * width / w;
                const int y0 = y * height / h, y1 = (y + 1) * height / h;
                for (int c = 0; c < 4; ++c) {
                    unsigned int sum = 0;
                    for (int sy = y0; sy < y1; ++sy)
                        for (int sx = x0; sx < x1; ++sx)
                            sum += data[(static_cast<size_t>(sy) * width + sx) * 4 + c];
                    pixels[(static_cast<size_t>(y) * w + x) * 4 + c] = sum / ((x1 - x0) * (y1 - y0));
                }
            }
        }
        desc.data.mip_levels[desc.num_mipmaps++] = {pixels.data(), pixels.size()};
        data = pixels.data();
        width = w;
        height = h;
    }
    return createTextureView(sg_make_image(desc));
}

unsigned int loadTextureFromFile(const std::string& path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    auto* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "[Error] Texture failed to load: " << path << '\n';
        return createSolidColorTexture({1, 0, 1});
    }
    const auto texture = createTexture(data, width, height);
    stbi_image_free(data);
    return texture;
}

unsigned int loadHDRTexture(const std::string& path) {
    if (!sg_isvalid())
        return 0;
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    auto* data = stbi_loadf(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "[Error] HDR texture failed to load: " << path << '\n';
        return 0;
    }
    sg_image_desc desc{};
    desc.width = width;
    desc.height = height;
    desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
    desc.data.mip_levels[0] = {data, static_cast<size_t>(width) * height * 4 * sizeof(float)};
    const auto texture = createTextureView(sg_make_image(desc));
    stbi_image_free(data);
    return texture;
}
} // namespace knot
