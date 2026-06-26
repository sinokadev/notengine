// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 SinokaDev

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <knot/utility.h>

namespace knot {

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
    TextureMaterial(std::shared_ptr<Shader> s, unsigned int textureId) : Material(s), textureId(textureId) {
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
};

class AlphaMaterial : public TextureMaterial {
public:
    AlphaMaterial(std::shared_ptr<Shader> s, glm::vec3 color) : TextureMaterial(s, createSolidColorTexture(color)), color(color) {
    }

private:
    glm::vec3 color;
};

class PbrMaterial : public Material {
public:
    PbrMaterial(std::shared_ptr<Shader> s, 
                glm::vec3 albedoColor = glm::vec3(1.0f), 
                float metallicFactor = 0.0f, 
                float roughnessFactor = 0.5f, 
                float aoFactor = 1.0f,
                unsigned int albedoM = 0, 
                unsigned int metallicM = 0, 
                unsigned int roughnessM = 0, 
                unsigned int aoM = 0)
        : Material(s), 
          baseAlbedo(albedoColor), baseMetallic(metallicFactor), baseRoughness(roughnessFactor), baseAo(aoFactor) {
        
        // 1. Albedo 텍스처 처리
        if (albedoM != 0) {
            albedoMap = albedoM;
        } else {
            albedoMap = createSolidColorTexture(baseAlbedo);
            isAlbedoAllocated = true; // 내부에서 할당됨을 기록
        }
        
        // 2. Metallic 텍스처 처리
        if (metallicM != 0) {
            metallicMap = metallicM;
        } else {
            metallicMap = createSolidColorTexture(glm::vec3(baseMetallic));
            isMetallicAllocated = true;
        }
        
        // 3. Roughness 텍스처 처리
        if (roughnessM != 0) {
            roughnessMap = roughnessM;
        } else {
            roughnessMap = createSolidColorTexture(glm::vec3(baseRoughness));
            isRoughnessAllocated = true;
        }
        
        // 4. AO 텍스처 처리
        if (aoM != 0) {
            aoMap = aoM;
        } else {
            aoMap = createSolidColorTexture(glm::vec3(baseAo));
            isAoAllocated = true;
        }

        useAlbedoMap    = true;
        useMetallicMap  = true;
        useRoughnessMap = true;
        useAoMap        = true;
    }

    // 소멸자: 내부에서 생성한 단색 텍스처만 선택적으로 안전하게 해제
    ~PbrMaterial() {
        if (isAlbedoAllocated && albedoMap != 0)    glDeleteTextures(1, &albedoMap);
        if (isMetallicAllocated && metallicMap != 0)  glDeleteTextures(1, &metallicMap);
        if (isRoughnessAllocated && roughnessMap != 0) glDeleteTextures(1, &roughnessMap);
        if (isAoAllocated && aoMap != 0)       glDeleteTextures(1, &aoMap);
    }

    // 개별 텍스처 설정 메서드 (기존 내부 텍스처 누수 방지)
    void setAlbedoMap(unsigned int texID) { 
        if (isAlbedoAllocated && albedoMap != 0) {
            glDeleteTextures(1, &albedoMap);
            isAlbedoAllocated = false;
        }
        albedoMap = texID; 
        useAlbedoMap = true; 
    }
    
    void setMetallicMap(unsigned int texID) { 
        if (isMetallicAllocated && metallicMap != 0) {
            glDeleteTextures(1, &metallicMap);
            isMetallicAllocated = false;
        }
        metallicMap = texID; 
        useMetallicMap = true; 
    }

    void setRoughnessMap(unsigned int texID) { 
        if (isRoughnessAllocated && roughnessMap != 0) {
            glDeleteTextures(1, &roughnessMap);
            isRoughnessAllocated = false;
        }
        roughnessMap = texID; 
        useRoughnessMap = true; 
    }

    void setAoMap(unsigned int texID) { 
        if (isAoAllocated && aoMap != 0) {
            glDeleteTextures(1, &aoMap);
            isAoAllocated = false;
        }
        aoMap = texID; 
        useAoMap = true; 
    }

    void bind() override {
        if (!shader) return;

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
    }

public:
    unsigned int albedoMap;
    unsigned int metallicMap;
    unsigned int roughnessMap;
    unsigned int aoMap;

    bool useAlbedoMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useAoMap;

    glm::vec3 baseAlbedo;
    float baseMetallic;
    float baseRoughness;
    float baseAo;

private:
    // 자기가 직접 단색 텍스처를 해제해야 하는지 판별하는 플래그
    bool isAlbedoAllocated = false;
    bool isMetallicAllocated = false;
    bool isRoughnessAllocated = false;
    bool isAoAllocated = false;
};

struct Object {
    std::shared_ptr<Material> material;
    std::shared_ptr<Mesh> mesh;
    unsigned int id = 0;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    Object(std::shared_ptr<Mesh> m, std::shared_ptr<Material> mat) : mesh(std::move(m)), material(std::move(mat)) {
    }

    Object() : mesh(nullptr), material(nullptr) {
    }

    virtual ~Object() = default;

    glm::mat4 getWorldMatrix() const;

    virtual void move(glm::vec3 direction, float deltaTime) {
    }
    virtual void rotate(float xOffset, float yOffset, bool constrainPitch = true) {
    }
};

class Light : public Object {
public:
    glm::vec3 color;
    float intensity;

    // 생성자: Mesh와 Material은 Light에 필요 없으므로 nullptr로 부모 생성자 호출
    Light(glm::vec3 lightColor = glm::vec3(1.0f), float lightIntensity = 1.0f) : Object(), color(lightColor), intensity(lightIntensity) {
    }

    virtual ~Light() = default;
};

class PbrPointLight : public Light {
public:
    // 내부에 glm::vec3 color; 나 brightness 같은 변수가 없어야 합니다! (부모인 Light의 것을 사용)
    
    PbrPointLight(glm::vec3 pos = glm::vec3(0.0f), 
                  glm::vec3 col = glm::vec3(1.0f), 
                  float bright = 1.0f)
        : Light(col, bright) {
        this->position = pos;
    }

    virtual ~PbrPointLight() = default;
};
class DirLight : public Light {
public:
    // 방향성 조명은 '방향'이 핵심입니다.
    glm::vec3 direction;

    // Phong 조명 구성 요소
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    // 생성자
    DirLight(glm::vec3 dir = glm::vec3(-0.2f, -1.0f, -0.3f), // 아래를 향하는 디폴트 방향
                 glm::vec3 amb = glm::vec3(0.05f), glm::vec3 diff = glm::vec3(0.8f),
                 glm::vec3 spec = glm::vec3(1.0f))
        : // 매니저가 ID를 줄 것이므로 임시로 0 전달
          direction(dir), ambient(amb), diffuse(diff), specular(spec) {
        this->color = diff;
    }

    virtual ~DirLight() = default;
};
} // namespace knot
