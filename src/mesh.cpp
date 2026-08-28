#include <knot/mesh.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace knot {

Mesh::~Mesh() {
    if (glfwGetCurrentContext() != nullptr) {
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }
        if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
        }
    }
    vao = 0;
    vbo = 0;
    ebo = 0;
}

void Mesh::setup() {
    if (vertices.empty() || indices.empty()) {
        std::cerr << "[Error] Mesh::setup called with empty geometry" << std::endl;
        return;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Normal)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Tangent)));

    glBindVertexArray(0);
    indexCount = static_cast<unsigned int>(indices.size());
}

void Mesh::setupInstanceAttributes(unsigned int instanceVBO) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    constexpr GLuint INSTANCE_LOCATION = 4;

    for (GLuint i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(INSTANCE_LOCATION + i);

        glVertexAttribPointer(INSTANCE_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(sizeof(glm::vec4) * i));

        glVertexAttribDivisor(INSTANCE_LOCATION + i, 1);
    }

    glBindVertexArray(0);
}

} // namespace knot