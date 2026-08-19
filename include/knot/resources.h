// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <knot/utility.h>

namespace knot {

struct Frustum;

unsigned int createSolidColorTexture(glm::vec3 color);

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
    bool isReady() const {
        return vao != 0 && indexCount > 0;
    }
    void setupInstanceAttributes(unsigned int instanceVBO);
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
};

class Shader {
public:
    Shader(std::shared_ptr<ShaderSource> ss, unsigned int id);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool isValid() const {
        return valid;
    }

    void use();
    void set(const std::string& name, bool value) const;
    void set(const std::string& name, int value) const;
    void set(const std::string& name, float value) const;
    void set(const std::string& name, const glm::vec2& value) const;
    void set(const std::string& name, const glm::vec3& value) const;
    void set(const std::string& name, const glm::mat4& value) const;
    unsigned int getId() const;
    unsigned int getShaderProgram() const;

private:
    int uniformLocation(const std::string& name) const;

    bool valid = false;
    unsigned int shaderProgram = 0;
    unsigned int id = 0;
    mutable std::unordered_map<std::string, int> uniformLocations;
};

class PbrShader {
public:
    static ShaderSource GetSource();
};

class AlphaShader {
public:
    static ShaderSource GetSource();
};

class Material {
public:
    explicit Material(std::shared_ptr<Shader> shader) : shader(std::move(shader)) {
    }

    virtual ~Material() = default;

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
    TextureMaterial(std::shared_ptr<Shader> s, unsigned int textureId, bool ownsTexture = false)
        : Material(std::move(s)), textureId(textureId), ownsTexture(ownsTexture) {
    }

    ~TextureMaterial() override {
        if (ownsTexture && textureId != 0) {
            if (glfwGetCurrentContext() != nullptr) {
                glDeleteTextures(1, &textureId);
            }
            textureId = 0;
        }
    }

    void bind() override {
        if (!shader)
            return;

        shader->use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);

        shader->set("material.diffuse", 0);
    }

private:
    unsigned int textureId;
    bool ownsTexture = false;
};

class AlphaMaterial : public TextureMaterial {
public:
    AlphaMaterial(std::shared_ptr<Shader> s, glm::vec3 color)
        : TextureMaterial(std::move(s), createSolidColorTexture(color), true), color(color) {
    }

private:
    glm::vec3 color;
};

class PbrMaterial : public Material {
public:
    PbrMaterial(std::shared_ptr<Shader> s, glm::vec3 albedoColor = glm::vec3(1.0f), float metallicFactor = 0.0f, float roughnessFactor = 0.5f,
                float aoFactor = 1.0f, unsigned int albedoM = 0, unsigned int metallicM = 0, unsigned int roughnessM = 0, unsigned int aoM = 0,
                unsigned int normalM = 0)
        : Material(std::move(s)), albedoMap(0), metallicMap(0), roughnessMap(0), aoMap(0), normalMap(0), isAlbedoAllocated(false), isMetallicAllocated(false),
          isRoughnessAllocated(false), isAoAllocated(false), isNormalAllocated(false), baseAlbedo(albedoColor), baseMetallic(metallicFactor),
          baseRoughness(roughnessFactor), baseAo(aoFactor) {

        if (albedoM != 0) {
            albedoMap = albedoM;
        } else {
            albedoMap = createSolidColorTexture(baseAlbedo);
            isAlbedoAllocated = true;
        }

        if (metallicM != 0) {
            metallicMap = metallicM;
        } else {
            metallicMap = createSolidColorTexture(glm::vec3(baseMetallic));
            isMetallicAllocated = true;
        }

        if (roughnessM != 0) {
            roughnessMap = roughnessM;
        } else {
            roughnessMap = createSolidColorTexture(glm::vec3(baseRoughness));
            isRoughnessAllocated = true;
        }

        if (aoM != 0) {
            aoMap = aoM;
        } else {
            aoMap = createSolidColorTexture(glm::vec3(baseAo));
            isAoAllocated = true;
        }

        if (normalM != 0) {
            normalMap = normalM;
        } else {
            normalMap = createSolidColorTexture(glm::vec3(0.5f, 0.5f, 1.0f));
            isNormalAllocated = true;
        }
    }

    ~PbrMaterial() override {
        const bool hasContext = (glfwGetCurrentContext() != nullptr);
        if (isAlbedoAllocated && albedoMap != 0) {
            if (hasContext) {
                glDeleteTextures(1, &albedoMap);
            }
            albedoMap = 0;
            isAlbedoAllocated = false;
        }
        if (isMetallicAllocated && metallicMap != 0) {
            if (hasContext) {
                glDeleteTextures(1, &metallicMap);
            }
            metallicMap = 0;
            isMetallicAllocated = false;
        }
        if (isRoughnessAllocated && roughnessMap != 0) {
            if (hasContext) {
                glDeleteTextures(1, &roughnessMap);
            }
            roughnessMap = 0;
            isRoughnessAllocated = false;
        }
        if (isAoAllocated && aoMap != 0) {
            if (hasContext) {
                glDeleteTextures(1, &aoMap);
            }
            aoMap = 0;
            isAoAllocated = false;
        }
        if (isNormalAllocated && normalMap != 0) {
            if (hasContext) {
                glDeleteTextures(1, &normalMap);
            }
            normalMap = 0;
            isNormalAllocated = false;
        }
    }

    void setAlbedoMap(unsigned int texID) {
        if (isAlbedoAllocated && albedoMap != 0) {
            glDeleteTextures(1, &albedoMap);
            isAlbedoAllocated = false;
        }
        albedoMap = texID;
    }

    void setMetallicMap(unsigned int texID) {
        if (isMetallicAllocated && metallicMap != 0) {
            glDeleteTextures(1, &metallicMap);
            isMetallicAllocated = false;
        }
        metallicMap = texID;
    }

    void setRoughnessMap(unsigned int texID) {
        if (isRoughnessAllocated && roughnessMap != 0) {
            glDeleteTextures(1, &roughnessMap);
            isRoughnessAllocated = false;
        }
        roughnessMap = texID;
    }

    void setAoMap(unsigned int texID) {
        if (isAoAllocated && aoMap != 0) {
            glDeleteTextures(1, &aoMap);
            isAoAllocated = false;
        }
        aoMap = texID;
    }

    void setNormalMap(unsigned int texID) {
        if (isNormalAllocated && normalMap != 0) {
            glDeleteTextures(1, &normalMap);
            isNormalAllocated = false;
        }
        normalMap = texID;
    }

    void bind() override {
        if (!shader)
            return;

        shader->use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoMap);
        shader->set("material.albedoMap", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, metallicMap);
        shader->set("material.metallicMap", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, roughnessMap);
        shader->set("material.roughnessMap", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, aoMap);
        shader->set("material.aoMap", 3);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        shader->set("material.normalMap", 4);
    }

public:
    unsigned int albedoMap;
    unsigned int metallicMap;
    unsigned int roughnessMap;
    unsigned int aoMap;
    unsigned int normalMap;

    glm::vec3 baseAlbedo;
    float baseMetallic;
    float baseRoughness;
    float baseAo;

private:
    bool isAlbedoAllocated = false;
    bool isMetallicAllocated = false;
    bool isRoughnessAllocated = false;
    bool isAoAllocated = false;
    bool isNormalAllocated = false;
};

struct Model {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;

    glm::vec3 boundsCenter = glm::vec3(0.0f);
    float boundsRadius = 1.0f;

    Model() = default;

    Model(
        std::shared_ptr<Mesh> mesh,
        std::shared_ptr<Material> material
    );

    void calculateBounds();
};

class Transform {
public:
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 getWorldMatrix() const;

    glm::vec3 getFront() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
};

class Object : public Transform {
public:
    unsigned int id = 0;

    std::shared_ptr<Model> model;

    Object() = default;

    explicit Object(std::shared_ptr<Model> model)
        : model(std::move(model)) {
    }

    Object(
        std::shared_ptr<Mesh> mesh,
        std::shared_ptr<Material> material
    )
        : model(std::make_shared<Model>(
              std::move(mesh),
              std::move(material)
          )) {
    }

    virtual ~Object() = default;

    bool isVisible(const Frustum& frustum) const;
    bool isVisible(
        const Frustum& frustum,
        const glm::mat4& worldMatrix
    ) const;
};

class Light : public Transform {
public:
    unsigned int id = 0;

    glm::vec3 color{1.0f};
    float intensity = 1.0f;

    Light() = default;

    Light(
        glm::vec3 lightColor,
        float lightIntensity,
        glm::vec3 position = glm::vec3(0.0f)
    )
        : color(lightColor),
          intensity(lightIntensity) {
        this->position = position;
    }

    virtual ~Light() = default;
};

class PbrPointLight : public Light {
public:
    PbrPointLight(
        glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 col = glm::vec3(1.0f),
        float bright = 1.0f
    )
        : Light(col, bright, pos) {
    }
};

class DirLight : public Light {
public:
    glm::vec3 ambient{0.05f};
    glm::vec3 diffuse{0.8f};
    glm::vec3 specular{1.0f};

    glm::vec3 getDirection() const {
        return getFront();
    }
};

} // namespace knot
