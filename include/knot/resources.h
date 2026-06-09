#pragma once

#include <vector>
#include <sstream>
#include <iostream>
#include <memory>
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
        void setup();
    };

    struct Object {
        std::shared_ptr<Mesh> mesh;
        const unsigned int id;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        Object(std::shared_ptr<Mesh> m, unsigned int _id) : mesh(m), id(_id) {}
        
        glm::mat4 getWorldMatrix() const;
    };

    class ShaderSource {
    public:
        std::string vertexPath;
        std::string fragmentPath;
        
        std::string vertexSourceCode;
        std::string fragmentSourceCode;

        ShaderSource(std::string v, std::string f);

    private:
        std::string readFile(const std::string& path);
    };


    class Shader {
    public:
        Shader(ShaderSource &ss);
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

    class AlphaShader {
    public:
        // 엔진 내부 경로가 바뀔 때 여기서만 수정하면 됩니다.
        static ShaderSource GetSource() {
            return ShaderSource("assets/shaders/alpha.vert", "assets/shaders/alpha.frag");
        }
        
        // 편의를 위해 Shader 객체 자체를 바로 반환하는 함수
        static Shader Create() {
            ShaderSource ss = GetSource();
            return Shader(ss);
        }
    };
}