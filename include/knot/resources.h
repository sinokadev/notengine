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

    struct Mesh {
        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ebo = 0;
        
        unsigned int indexCount = 0;

        ~Mesh() {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }
    };

    struct Object {
        Mesh* mesh;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        glm::mat4 getWorldMatrix() const {
            glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
            
            glm::mat4 rot = glm::mat4_cast(rotation);
            
            glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);

            return trans * rot * sca;
        }
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