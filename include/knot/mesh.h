#pragma once

#include <glm/glm.hpp>
#include <sokol/sokol_gfx.h>
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

/** @brief CPU mesh data and its sokol_gfx vertex/index buffers. */
struct Mesh {
    /** @brief Vertex data retained on the CPU. */
    std::vector<Vertex> vertices;
    /** @brief Triangle index data retained on the CPU. */
    std::vector<unsigned int> indices;

    /** @brief Local bounding-box center, refreshed by setup(), for transparency sorting. */
    glm::vec3 boundsCenter{0.0f};

    sg_buffer vertexBuffer{};
    sg_buffer indexBuffer{};

    Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    /** @brief Number of indices submitted for drawing. */
    unsigned int indexCount = 0;

    /** @brief Releases owned GPU buffers. */
    ~Mesh();
    /** @brief Uploads vertices and indices. */
    void setup();
    /** @brief Reports whether GPU buffers were created and contain indices. */
    bool isReady() const {
        return sg_isvalid() && sg_query_buffer_state(vertexBuffer) == SG_RESOURCESTATE_VALID &&
               sg_query_buffer_state(indexBuffer) == SG_RESOURCESTATE_VALID && indexCount > 0;
    }
};

} // namespace knot
