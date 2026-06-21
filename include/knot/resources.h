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

class PongShader {
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

class PongMaterial : public Material {
public:
    // 생성자: 부모 클래스 Material(shader)을 호출하여 셰이더를 등록합니다.
    PongMaterial(std::shared_ptr<Shader> s, glm::vec3 diffColor, glm::vec3 specColor, float shininess, unsigned int diffMap = 0,
                 unsigned int specMap = 0, unsigned int normMap = 0, unsigned int roughMap = 0)
        : Material(s), baseDiffuse(diffColor), baseSpecular(specColor), baseShininess(shininess), diffuseMap(diffMap), specularMap(specMap),
          normalMap(normMap), roughnessMap(roughMap) {
        // 텍스처 ID가 0이 아니면(유효한 텍스처가 들어오면) 자동으로 사용 플래그를 true로 설정합니다.
        useDiffuseMap = (diffuseMap != 0);
        useSpecularMap = (specularMap != 0);
        useNormalMap = (normalMap != 0);
        useRoughnessMap = (roughnessMap != 0);
    }

    // 텍스처 설정 메서드들 (필요에 따라 사용)
    void setDiffuseMap(unsigned int texID) {
        diffuseMap = texID;
        useDiffuseMap = true;
    }
    void setSpecularMap(unsigned int texID) {
        specularMap = texID;
        useSpecularMap = true;
    }
    void setNormalMap(unsigned int texID) {
        normalMap = texID;
        useNormalMap = true;
    }
    void setRoughnessMap(unsigned int texID) {
        roughnessMap = texID;
        useRoughnessMap = true;
    }

    // 렌더러가 오브젝트를 그리기 직전에 호출할 바인딩 함수
    void bind() override {
        if (!shader)
            return;

        // 1. 셰이더 프로그램 활성화
        shader->use();

        // 2. 텍스처 맵 바인딩 (활성화된 텍스처 유닛 0~3번 순차 배정)
        if (useDiffuseMap) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap);
            shader->set("material.diffuseMap", 0);
        }
        if (useSpecularMap) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, specularMap);
            shader->set("material.specularMap", 1);
        }
        if (useNormalMap) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, normalMap);
            shader->set("material.normalMap", 2);
        }
        if (useRoughnessMap) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, roughnessMap);
            shader->set("material.roughnessMap", 3);
        }

        // 3. 텍스처 사용 여부 플래그(bool) 전송
        shader->set("material.useDiffuseMap", useDiffuseMap);
        shader->set("material.useSpecularMap", useSpecularMap);
        shader->set("material.useNormalMap", useNormalMap);
        shader->set("material.useRoughnessMap", useRoughnessMap);

        // 4. 기본 색상 및 속성 데이터(vec3, float) 전송
        shader->set("material.baseDiffuse", baseDiffuse);
        shader->set("material.baseSpecular", baseSpecular);
        shader->set("material.baseShininess", baseShininess);
    }

public:
    // GLSL 구조체 멤버와 1:1 매핑되는 멤버 변수들
    unsigned int diffuseMap;
    unsigned int specularMap;
    unsigned int normalMap;
    unsigned int roughnessMap;

    bool useDiffuseMap;
    bool useSpecularMap;
    bool useRoughnessMap;
    bool useNormalMap;

    glm::vec3 baseDiffuse;
    glm::vec3 baseSpecular;
    float baseShininess;
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

class PongPointLight : public Light {
public:
    // Phong 조명 구성 요소
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    // 거리 감쇄(Attenuation) 계수
    float constant;
    float linear;
    float quadratic;

    // 생성자
    PongPointLight(glm::vec3 amb = glm::vec3(0.05f), glm::vec3 diff = glm::vec3(0.8f), glm::vec3 spec = glm::vec3(1.0f), float cons = 1.0f,
                   float lin = 0.09f, float quad = 0.032f)
        : ambient(amb), diffuse(diff), specular(spec), constant(cons), linear(lin), quadratic(quad) {

        // 필요하다면 Light 클래스의 기본 color나 intensity도 여기서 초기화할 수 있습니다.
        this->color = diff;
    }
};

class PongDirLight : public Light { // 앞서 정의한 PongLight 상속
public:
    // 방향성 조명은 '방향'이 핵심입니다.
    glm::vec3 direction;

    // Phong 조명 구성 요소
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    // 생성자
    PongDirLight(glm::vec3 dir = glm::vec3(-0.2f, -1.0f, -0.3f), // 아래를 향하는 디폴트 방향
                 glm::vec3 amb = glm::vec3(0.05f), glm::vec3 diff = glm::vec3(0.8f),
                 glm::vec3 spec = glm::vec3(1.0f))
        : // 매니저가 ID를 줄 것이므로 임시로 0 전달
          direction(dir), ambient(amb), diffuse(diff), specular(spec) {
        this->color = diff;
    }

    virtual ~PongDirLight() = default;
};
} // namespace knot
