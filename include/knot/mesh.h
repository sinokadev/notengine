// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

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

/** @brief CPU mesh data and its OpenGL vertex/index buffers. */
struct Mesh {
    /** @brief Vertex data retained on the CPU. */
    std::vector<Vertex> vertices;
    /** @brief Triangle index data retained on the CPU. */
    std::vector<unsigned int> indices;

    /** @brief OpenGL vertex-array object. */
    unsigned int vao = 0;
    /** @brief OpenGL vertex-buffer object. */
    unsigned int vbo = 0;
    /** @brief OpenGL element-buffer object. */
    unsigned int ebo = 0;
    /** @brief Number of indices submitted for drawing. */
    unsigned int indexCount = 0;

    /** @brief Deletes owned OpenGL buffers when a context is current. */
    ~Mesh();
    /** @brief Uploads vertices and indices and configures the mesh VAO. */
    void setup();
    /** @brief Reports whether GPU buffers were created and contain indices. */
    bool isReady() const {
        return vao != 0 && indexCount > 0;
    }
    /** @brief Configures mat4 per-instance attributes from an OpenGL VBO.
     *  @param instanceVBO Buffer containing one model matrix per instance. */
    void setupInstanceAttributes(unsigned int instanceVBO);
};

} // namespace knot
