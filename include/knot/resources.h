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
#include <knot/mesh.h>

namespace knot {

struct Frustum;

/** @brief Creates a 1x1 RGBA texture from a color. */
unsigned int createSolidColorTexture(glm::vec3 color);

/** @brief Sets the base directory used by built-in asset paths. */
void setAssetRoot(const std::string& root);
/** @brief Returns the configured base directory for engine assets. */
const std::string& getAssetRoot();

/** @brief Loads vertex and fragment shader source files. */
class ShaderSource {
public:
    /** @brief Path used to load the vertex shader source. */
    std::string vertexPath;
    /** @brief Path used to load the fragment shader source. */
    std::string fragmentPath;
    /** @brief Loaded vertex shader source text. */
    std::string vertexSourceCode;
    /** @brief Loaded fragment shader source text. */
    std::string fragmentSourceCode;

    /** @brief Loads source text from the supplied vertex and fragment paths. */
    ShaderSource(std::string v, std::string f);
    /** @brief Reports whether both shader source strings were loaded. */
    bool isValid() const;

private:
};

/** @brief Compiled OpenGL shader program with cached uniform locations. */
class Shader {
public:
    /** @brief Compiles and links sources into a program associated with @p id. */
    Shader(std::shared_ptr<ShaderSource> ss, unsigned int id);
    /** @brief Deletes the OpenGL program when a context is current. */
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    /** @brief Reports whether shader compilation and linking succeeded. */
    bool isValid() const {
        return valid;
    }

    /** @brief Binds this program for subsequent draw calls. */
    void use();
    /** @name Uniform setters
     *  @brief Set a uniform in the currently linked program by name. */
    ///@{
    /** @brief Sets a boolean uniform. */
    void set(const std::string& name, bool value) const;
    /** @brief Sets an integer uniform. */
    void set(const std::string& name, int value) const;
    /** @brief Sets a floating-point uniform. */
    void set(const std::string& name, float value) const;
    /** @brief Sets a two-component vector uniform. */
    void set(const std::string& name, const glm::vec2& value) const;
    /** @brief Sets a three-component vector uniform. */
    void set(const std::string& name, const glm::vec3& value) const;
    /** @brief Sets a 4x4 matrix uniform. */
    void set(const std::string& name, const glm::mat4& value) const;
    ///@}
    /** @brief Returns the resource-manager ID assigned to this shader. */
    unsigned int getId() const;
    /** @brief Returns the underlying OpenGL program ID. */
    unsigned int getShaderProgram() const;

private:
    int uniformLocation(const std::string& name) const;

    bool valid = false;
    unsigned int shaderProgram = 0;
    unsigned int id = 0;
    mutable std::unordered_map<std::string, int> uniformLocations;
};

/** @brief Factory for the engine's built-in PBR shader sources. */
class PbrShader {
public:
    /** @brief Returns paths for the built-in PBR vertex and fragment shaders. */
    static ShaderSource GetSource();
};

/** @brief Factory for the engine's built-in alpha shader sources. */
class AlphaShader {
public:
    /** @brief Returns paths for the built-in alpha vertex and fragment shaders. */
    static ShaderSource GetSource();
};

/** @brief Base material that binds an associated shader program. */
class Material {
public:
    /** @brief Creates a material using @p shader. */
    explicit Material(std::shared_ptr<Shader> shader) : shader(std::move(shader)) {
    }

    virtual ~Material() = default;

    /** @brief Binds the material's shader program when available. */
    virtual void bind() {
        if (shader) {
            shader->use();
        }
    }

    /** @brief Returns the shader used by this material. */
    std::shared_ptr<Shader> getShader() const {
        return shader;
    }

protected:
    /** @brief Shader program used when binding this material. */
    std::shared_ptr<Shader> shader;
};

/** @brief Diffuse texture material for the built-in alpha shader. */
class TextureMaterial : public Material {
public:
    /** @brief Creates a material using a 2D texture.
     *  @param s Shader to use.
     *  @param textureId OpenGL 2D texture ID.
     *  @param ownsTexture Whether this material deletes @p textureId on destruction. */
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

    /** @brief Binds the texture to unit 0 and sets material.diffuse. */
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

/** @brief Solid-color texture material that owns its generated texture. */
class AlphaMaterial : public TextureMaterial {
public:
    /** @brief Creates a material backed by an owned 1x1 texture of @p color. */
    AlphaMaterial(std::shared_ptr<Shader> s, glm::vec3 color) : TextureMaterial(std::move(s), createSolidColorTexture(color), true), color(color) {
    }

private:
    glm::vec3 color;
};

/** @brief Material for the built-in physically based renderer. */
class PbrMaterial : public Material {
public:
    /** @brief Creates a PBR material, generating 1x1 fallback maps for zero IDs.
     *  @param s Shader to bind.
     *  @param albedoColor Fallback base color.
     *  @param metallicFactor Fallback metallic value.
     *  @param roughnessFactor Fallback roughness value.
     *  @param aoFactor Fallback ambient-occlusion value.
     *  @param albedoM Optional albedo texture ID.
     *  @param metallicM Optional metallic texture ID.
     *  @param roughnessM Optional roughness texture ID.
     *  @param aoM Optional ambient-occlusion texture ID.
     *  @param normalM Optional normal texture ID. */
    PbrMaterial(std::shared_ptr<Shader> s, glm::vec3 albedoColor = glm::vec3(1.0f), float metallicFactor = 0.0f, float roughnessFactor = 0.5f,
                float aoFactor = 1.0f, unsigned int albedoM = 0, unsigned int metallicM = 0, unsigned int roughnessM = 0, unsigned int aoM = 0,
                unsigned int normalM = 0)
        : Material(std::move(s)), albedoMap(0), metallicMap(0), roughnessMap(0), aoMap(0), normalMap(0), isAlbedoAllocated(false),
          isMetallicAllocated(false), isRoughnessAllocated(false), isAoAllocated(false), isNormalAllocated(false), baseAlbedo(albedoColor),
          baseMetallic(metallicFactor), baseRoughness(roughnessFactor), baseAo(aoFactor) {

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

    /** @brief Replaces the albedo map; an owned fallback texture is released. */
    void setAlbedoMap(unsigned int texID) {
        if (isAlbedoAllocated && albedoMap != 0) {
            glDeleteTextures(1, &albedoMap);
            isAlbedoAllocated = false;
        }
        albedoMap = texID;
    }

    /** @brief Replaces the metallic map; an owned fallback texture is released. */
    void setMetallicMap(unsigned int texID) {
        if (isMetallicAllocated && metallicMap != 0) {
            glDeleteTextures(1, &metallicMap);
            isMetallicAllocated = false;
        }
        metallicMap = texID;
    }

    /** @brief Replaces the roughness map; an owned fallback texture is released. */
    void setRoughnessMap(unsigned int texID) {
        if (isRoughnessAllocated && roughnessMap != 0) {
            glDeleteTextures(1, &roughnessMap);
            isRoughnessAllocated = false;
        }
        roughnessMap = texID;
    }

    /** @brief Replaces the ambient-occlusion map; an owned fallback texture is released. */
    void setAoMap(unsigned int texID) {
        if (isAoAllocated && aoMap != 0) {
            glDeleteTextures(1, &aoMap);
            isAoAllocated = false;
        }
        aoMap = texID;
    }

    /** @brief Replaces the normal map; an owned fallback texture is released. */
    void setNormalMap(unsigned int texID) {
        if (isNormalAllocated && normalMap != 0) {
            glDeleteTextures(1, &normalMap);
            isNormalAllocated = false;
        }
        normalMap = texID;
    }

    /** @brief Binds PBR maps to texture units 0 through 4. */
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
    /** @brief Albedo texture ID. */
    unsigned int albedoMap;
    /** @brief Metallic-factor texture ID. */
    unsigned int metallicMap;
    /** @brief Roughness-factor texture ID. */
    unsigned int roughnessMap;
    /** @brief Ambient-occlusion texture ID. */
    unsigned int aoMap;
    /** @brief Tangent-space normal texture ID. */
    unsigned int normalMap;

    /** @brief Fallback albedo used when no map is provided. */
    glm::vec3 baseAlbedo;
    /** @brief Fallback metallic value used when no map is provided. */
    float baseMetallic;
    /** @brief Fallback roughness value used when no map is provided. */
    float baseRoughness;
    /** @brief Fallback ambient-occlusion value used when no map is provided. */
    float baseAo;

private:
    bool isAlbedoAllocated = false;
    bool isMetallicAllocated = false;
    bool isRoughnessAllocated = false;
    bool isAoAllocated = false;
    bool isNormalAllocated = false;
};

/** @brief Renderable mesh/material pair with a local bounding sphere. */
struct Model {
    /** @brief Mesh geometry to draw. */
    std::shared_ptr<Mesh> mesh;
    /** @brief Material applied to the mesh. */
    std::shared_ptr<Material> material;

    /** @brief Center of the mesh's local bounding sphere. */
    glm::vec3 boundsCenter = glm::vec3(0.0f);
    /** @brief Radius of the mesh's local bounding sphere. */
    float boundsRadius = 1.0f;

    Model() = default;

    /** @brief Creates a model and calculates bounds from its mesh vertices. */
    Model(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

    /** @brief Recalculates the local bounding sphere from mesh vertices. */
    void calculateBounds();
};

/** @brief Position, quaternion rotation, and scale in world space. */
class Transform {
public:
    /** @brief World-space translation. */
    glm::vec3 position{0.0f};
    /** @brief World-space orientation. */
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    /** @brief Per-axis scale. */
    glm::vec3 scale{1.0f};

    /** @brief Returns translation * rotation * scale. */
    glm::mat4 getWorldMatrix() const;

    /** @brief Returns the rotation-derived forward direction. */
    glm::vec3 getFront() const;
    /** @brief Returns the rotation-derived right direction. */
    glm::vec3 getRight() const;
    /** @brief Returns the rotation-derived up direction. */
    glm::vec3 getUp() const;
};

/** @brief A transformable scene entity that references a render model. */
class Object : public Transform {
public:
    /** @brief Object-manager ID, or 0 before registration. */
    unsigned int id = 0;

    /** @brief Geometry and material drawn for this object. */
    std::shared_ptr<Model> model;

    Object() = default;

    /** @brief Creates an object from an existing model. */
    explicit Object(std::shared_ptr<Model> model) : model(std::move(model)) {
    }

    /** @brief Creates an object from mesh geometry and a material. */
    Object(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) : model(std::make_shared<Model>(std::move(mesh), std::move(material))) {
    }

    virtual ~Object() = default;

    /** @brief Tests the model bounds against @p frustum using this transform. */
    bool isVisible(const Frustum& frustum) const;
    /** @brief Tests the model bounds against @p frustum using @p worldMatrix. */
    bool isVisible(const Frustum& frustum, const glm::mat4& worldMatrix) const;
};

/** @brief Base class for transformable scene lights. */
class Light : public Transform {
public:
    /** @brief Light-manager ID, or 0 before registration. */
    unsigned int id = 0;

    /** @brief RGB light color. */
    glm::vec3 color{1.0f};
    /** @brief Brightness multiplier. */
    float intensity = 1.0f;

    Light() = default;

    /** @brief Creates a light with color, intensity, and position. */
    Light(glm::vec3 lightColor, float lightIntensity, glm::vec3 position = glm::vec3(0.0f)) : color(lightColor), intensity(lightIntensity) {
        this->position = position;
    }

    virtual ~Light() = default;
};

/** @brief Omnidirectional point light used by the PBR renderer. */
class PbrPointLight : public Light {
public:
    /** @brief Creates a point light at @p pos with color @p col and brightness @p bright. */
    PbrPointLight(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 col = glm::vec3(1.0f), float bright = 1.0f) : Light(col, bright, pos) {
    }
};

/** @brief Directional light with separate ambient, diffuse, and specular terms. */
class DirLight : public Light {
public:
    glm::vec3 ambient{0.05f};
    glm::vec3 diffuse{0.8f};
    glm::vec3 specular{1.0f};

    DirLight(glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3 ambient = glm::vec3(0.05f), glm::vec3 diffuse = glm::vec3(0.8f),
             glm::vec3 specular = glm::vec3(1.0f))
        : ambient(ambient), diffuse(diffuse), specular(specular) {
        if (glm::length(direction) > 0.0f) {
            rotation = glm::quatLookAt(glm::normalize(direction), glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }

    /** @brief Returns the light direction derived from its rotation. */
    glm::vec3 getDirection() const {
        return getFront();
    }
};

} // namespace knot
