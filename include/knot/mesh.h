#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace knot {

/** @brief Vertex data used by Knot meshes.
 *
 * Position, texture coordinates, normal, and tangent are uploaded as vertex
 * attributes 0 through 3 respectively. */
struct Vertex {
    /** @brief Object-space vertex position. */
    glm::vec3 Position;
    /** @brief Texture coordinates. */
    glm::vec2 TexCoords;
    /** @brief Object-space surface normal. */
    glm::vec3 Normal;
    /** @brief Object-space tangent. */
    glm::vec3 Tangent;
};

/** @brief CPU mesh data and its Vulkan vertex/index buffers. */
struct Mesh {
    /** @brief Vertex data retained on the CPU. */
    std::vector<Vertex> vertices;
    /** @brief Triangle index data retained on the CPU. */
    std::vector<unsigned int> indices;

    /** @brief Renderer-managed Vulkan geometry handle. */
    unsigned int gpuId = 0;
    Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    /** @brief Number of indices submitted for drawing. */
    unsigned int indexCount = 0;

    /** @brief Deletes owned Vulkan buffers while the renderer is alive. */
    ~Mesh();
    /** @brief Uploads vertices and indices and creates GPU buffers. */
    void setup();
    /** @brief Reports whether GPU buffers were created and contain indices. */
    bool isReady() const {
        return gpuId != 0 && indexCount > 0;
    }
};

} // namespace knot
