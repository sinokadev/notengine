#include <knot/utility/cubemap_baker.h>
#include "vulkan_internal.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace knot {
namespace {
constexpr float pi = 3.14159265358979323846f;
float radicalInverse(unsigned bits) {
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555u) << 1) | ((bits & 0xaaaaaaaau) >> 1);
    bits = ((bits & 0x33333333u) << 2) | ((bits & 0xccccccccu) >> 2);
    bits = ((bits & 0x0f0f0f0fu) << 4) | ((bits & 0xf0f0f0f0u) >> 4);
    bits = ((bits & 0x00ff00ffu) << 8) | ((bits & 0xff00ff00u) >> 8);
    return float(bits) * 2.3283064365386963e-10f;
}
glm::vec3 direction(int face, float u, float v) {
    switch (face) {
    case 0:
        return glm::normalize(glm::vec3(1, -v, -u));
    case 1:
        return glm::normalize(glm::vec3(-1, -v, u));
    case 2:
        return glm::normalize(glm::vec3(u, 1, v));
    case 3:
        return glm::normalize(glm::vec3(u, -1, -v));
    case 4:
        return glm::normalize(glm::vec3(u, -v, 1));
    default:
        return glm::normalize(glm::vec3(-u, -v, -1));
    }
}
glm::vec3 pixel(const vk::Texture& t, int face, int x, int y) {
    x = std::clamp(x, 0, int(t.image.width) - 1);
    y = std::clamp(y, 0, int(t.image.height) - 1);
    size_t i = ((size_t(face) * t.image.height + y) * t.image.width + x) * 4;
    return {t.pixels[i], t.pixels[i + 1], t.pixels[i + 2]};
}
glm::vec3 sample(const vk::Texture& t, glm::vec3 n) {
    float u, v;
    int face = 0;
    if (t.image.layers == 1) {
        u = std::atan2(n.z, n.x) / (2 * pi) + .5f;
        v = std::asin(std::clamp(n.y, -1.f, 1.f)) / pi + .5f;
    } else {
        auto a = glm::abs(n);
        if (a.x >= a.y && a.x >= a.z) {
            face = n.x > 0 ? 0 : 1;
            u = (n.x > 0 ? -n.z : n.z) / a.x;
            v = -n.y / a.x;
        } else if (a.y >= a.z) {
            face = n.y > 0 ? 2 : 3;
            u = n.x / a.y;
            v = (n.y > 0 ? n.z : -n.z) / a.y;
        } else {
            face = n.z > 0 ? 4 : 5;
            u = (n.z > 0 ? n.x : -n.x) / a.z;
            v = -n.y / a.z;
        }
        u = u * .5f + .5f;
        v = v * .5f + .5f;
    }
    float x = u * t.image.width - .5f, y = v * t.image.height - .5f;
    int ix = int(std::floor(x)), iy = int(std::floor(y));
    return glm::mix(glm::mix(pixel(t, face, ix, iy), pixel(t, face, ix + 1, iy), x - ix),
                    glm::mix(pixel(t, face, ix, iy + 1), pixel(t, face, ix + 1, iy + 1), x - ix), y - iy);
}
unsigned bake(unsigned source, int size, int mode) {
    auto& c = vk::context();
    auto it = c.textures.find(source);
    if (it == c.textures.end())
        return 0;
    if (size <= 0 || size > 4096)
        throw std::invalid_argument("Invalid cubemap face size");
    const auto& t = it->second;
    int levels = mode == 2 ? std::min(5, int(std::floor(std::log2(size))) + 1) : 1;
    std::vector<float> pixels;
    constexpr unsigned samples = 128;
    for (int level = 0; level < levels; ++level) {
        int w = std::max(1, size >> level);
        float roughness = levels > 1 ? float(level) / (levels - 1) : 0;
        for (int face = 0; face < 6; ++face)
            for (int y = 0; y < w; ++y)
                for (int x = 0; x < w; ++x) {
                    glm::vec3 n = direction(face, 2 * (x + .5f) / w - 1, 2 * (y + .5f) / w - 1), color(0);
                    float weight = 0;
                    if (mode == 0 || (mode == 2 && level == 0))
                        color = sample(t, n);
                    else {
                        glm::vec3 up = std::abs(n.z) < .999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
                        glm::vec3 tangent = glm::normalize(glm::cross(up, n)), bitangent = glm::cross(n, tangent);
                        for (unsigned i = 0; i < samples; ++i) {
                            float xi = float(i) / samples, yi = radicalInverse(i), phi = 2 * pi * xi;
                            float a = roughness * roughness;
                            float ct = mode == 1 ? std::sqrt(1 - yi) : std::sqrt((1 - yi) / (1 + (a * a - 1) * yi));
                            float st = std::sqrt(std::max(0.f, 1 - ct * ct));
                            glm::vec3 h = glm::normalize(tangent * (std::cos(phi) * st) + bitangent * (std::sin(phi) * st) + n * ct);
                            glm::vec3 l = mode == 1 ? h : glm::normalize(2.f * glm::dot(n, h) * h - n);
                            float nl = std::max(glm::dot(n, l), 0.f);
                            float contribution = mode == 1 ? 1.f : nl;
                            if (contribution > 0) {
                                color += sample(t, l) * contribution;
                                weight += contribution;
                            }
                        }
                        color /= std::max(weight, .0001f);
                    }
                    pixels.insert(pixels.end(), {color.r, color.g, color.b, 1.f});
                }
    }
    return c.upload(pixels, size, size, 6, levels);
}
} // namespace
unsigned bakeHDRMapToCubemap(unsigned id, int size) {
    return bake(id, size, 0);
}
unsigned bakeCubemapToIrradianceMap(unsigned id, int size) {
    return bake(id, size, 1);
}
unsigned bakeCubemapToPrefilterMap(unsigned id, int size) {
    return bake(id, size, 2);
}
} // namespace knot
