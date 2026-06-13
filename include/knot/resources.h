#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <knot/utility.h>

namespace knot {

    void setAssetRoot(const std::string& root);
    const std::string& getAssetRoot();

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
        bool isReady() const { return vao != 0 && indexCount > 0; }
    };

    class ShaderSource {
    public:
        std::string vertexPath;
        std::string fragmentPath;
        std::string vertexSourceCode;
        std::string fragmentSourceCode;

        ShaderSource(std::string v, std::string f);
        bool isValid() const;

    private:
        std::string readFile(const std::string& path);
    };

    class Shader {
    public:
        Shader(std::shared_ptr<ShaderSource> ss, unsigned int id);
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        bool isValid() const { return valid; }

        void use();
        void set(const std::string& name, bool value) const;
        void set(const std::string& name, int value) const;
        void set(const std::string& name, float value) const;
        void set(const std::string& name, const glm::vec2& value) const;
        void set(const std::string& name, const glm::vec3& value) const;
        void set(const std::string& name, const glm::mat4& value) const;
        unsigned int get_id() const;

    private:
        int uniformLocation(const std::string& name) const;

        bool valid = false;
        unsigned int shaderProgram = 0;
        unsigned int id = 0;
        mutable std::unordered_map<std::string, int> uniformLocations;
    };

    class AlphaShader {
    public:
        static ShaderSource GetSource();
    };

    class Material {
    public:
        explicit Material(std::shared_ptr<Shader> shader) : shader(std::move(shader)) {}

        virtual void bind() {
            if (shader) {
                shader->use();
            }
        }

        std::shared_ptr<Shader> getShader() const {
            return shader;
        }

    protected:
        std::shared_ptr<Shader> shader;
    };

    class TextureMaterial : public Material {
    public:
        TextureMaterial(std::shared_ptr<Shader> s, unsigned int textureId)
            : Material(s), textureId(textureId) {}

        void bind() override {
            if (!shader) return;

            shader->use();
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            
            shader->set("material.diffuse", 0); 
        }

    private:
        unsigned int textureId;
    };

    class AlphaMaterial : public TextureMaterial {
    public:
        AlphaMaterial(std::shared_ptr<Shader> s, glm::vec3 color)
            : TextureMaterial(s, createSolidColorTexture(color)), color(color) {
        }

    private:
        glm::vec3 color;
    };

    struct Object {
        std::shared_ptr<Material> material;
        std::shared_ptr<Mesh> mesh;
        const unsigned int id;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        Object(std::shared_ptr<Mesh> m, std::shared_ptr<Material> mat, unsigned int objectId)
            : mesh(std::move(m)), material(std::move(mat)), id(objectId) {}

        glm::mat4 getWorldMatrix() const;
    };
}
