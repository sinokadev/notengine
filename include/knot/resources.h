#pragma once

#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace knot {
    struct Vertex {
        glm::vec3 Position;
        glm::vec2 TexCoords;
        glm::vec3 Normal;
        glm::vec3 Tangent;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ebo = 0;
        
        unsigned int indexCount = 0;

        ~Mesh();
    };

    struct Object {
        Mesh* mesh;
        const unsigned int id;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        Object(Mesh* m, unsigned int _id) : mesh(m), id(_id) {}
        glm::mat4 getWorldMatrix() const;
        
    };

    class Shader {
    public:
        Shader(std::string vertex_path, std::string fragment_path);
        void use();
        void set(const std::string &name, bool value) const;
        void set(const std::string &name, int value) const;
        void set(const std::string &name, float value) const;
        void set(const std::string &name, const glm::vec2 &value) const;
        void set(const std::string &name, const glm::vec3 &value) const;
        void set(const std::string &name, const glm::mat4 &value) const;
        void set(int location, bool value) const;
        void set(int location, int value) const;
        void set(int location, float value) const;
        void set(int location, const glm::vec2 &value) const;
        void set(int location, const glm::vec3 &value) const;
        void set(int location, const glm::mat4 &value) const;
        unsigned int get_id() const;
        void destroy();

    private:
        unsigned int shader_program;
    };

}