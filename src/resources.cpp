#include <knot/resources.h>
#include <knot/camera.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GeneratedShaders.h"

namespace knot {

unsigned int createSolidColorTexture(glm::vec3 color);

namespace {

#ifdef NOTENGINE_ASSET_ROOT
std::string assetRoot = []() {
    std::string root = NOTENGINE_ASSET_ROOT;
    if (!root.empty() && root.back() != '/') {
        root += '/';
    }
    return root;
}();
#else
std::string assetRoot = "";
#endif

unsigned int compileShader(unsigned int type, const char* source, const char* label) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    char log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "[Error] Failed to compile " << label << " shader\n" << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

} // namespace

void setAssetRoot(const std::string& root) {
    assetRoot = root;
    if (!assetRoot.empty() && assetRoot.back() != '/') {
        assetRoot += '/';
    }
}

const std::string& getAssetRoot() {
    return assetRoot;
}

ShaderSource::ShaderSource(std::string v, std::string f) : vertexPath(v), fragmentPath(f) {
    // 1. 파일 이름만 추출 (디렉토리 경로 제거)
    std::string vName = std::filesystem::path(v).filename().string();
    std::string fName = std::filesystem::path(f).filename().string();

    // 2. 파일 이름으로 Registry 검색
    if (Shaders::Registry.count(vName)) {
        vertexSourceCode = Shaders::Registry.at(vName);
    } else {
        std::cerr << "[Error] Failed to find shader in Registry: " << vName << std::endl;
    }

    if (Shaders::Registry.count(fName)) {
        fragmentSourceCode = Shaders::Registry.at(fName);
    } else {
        std::cerr << "[Error] Failed to find shader in Registry: " << fName << std::endl;
    }
}
bool ShaderSource::isValid() const {
    return !vertexSourceCode.empty() && !fragmentSourceCode.empty();
}

Shader::Shader(std::shared_ptr<ShaderSource> ss, unsigned int shaderId) : id(shaderId) {
    if (!ss || !ss->isValid()) {
        return;
    }

    const char* vertexCode = ss->vertexSourceCode.c_str();
    const char* fragmentCode = ss->fragmentSourceCode.c_str();

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode, "vertex");
    if (vertexShader == 0) {
        return;
    }

    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode, "fragment");
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success = 0;
    char log[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cerr << "[Error] Failed to link shaders\n" << log << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    valid = true;
}

Shader::~Shader() {
    if (shaderProgram != 0) {
        if (glfwGetCurrentContext() != nullptr) {
            glDeleteProgram(shaderProgram);
        }
        shaderProgram = 0;
    }
}

void Shader::use() {
    if (valid) {
        glUseProgram(shaderProgram);
    }
}

int Shader::uniformLocation(const std::string& name) const {
    auto cached = uniformLocations.find(name);
    if (cached != uniformLocations.end()) {
        return cached->second;
    }

    const int location = glGetUniformLocation(shaderProgram, name.c_str());
    uniformLocations.emplace(name, location);
    return location;
}

void Shader::set(const std::string& name, bool value) const {
    const int location = uniformLocation(name);
    if (location >= 0) {
        glUniform1i(location, static_cast<int>(value));
    }
}

void Shader::set(const std::string& name, int value) const {
    const int location = uniformLocation(name);
    if (location >= 0) {
        glUniform1i(location, value);
    }
}

void Shader::set(const std::string& name, float value) const {
    const int location = uniformLocation(name);
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void Shader::set(const std::string& name, const glm::vec2& value) const {
    const int location = uniformLocation(name);
    if (location >= 0) {
        glUniform2fv(location, 1, &value[0]);
    }
}

void Shader::set(const std::string& name, const glm::vec3& value) const {
    const int location = uniformLocation(name);
    if (location >= 0) {
        glUniform3fv(location, 1, &value[0]);
    }
}

void Shader::set(const std::string& name, const glm::mat4& value) const {
    const int location = uniformLocation(name);
    if (location >= 0) {
        glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    }
}

unsigned int Shader::getId() const {
    return id;
}

unsigned int Shader::getShaderProgram() const {
    return shaderProgram;
}

ShaderSource AlphaShader::GetSource() {
    return ShaderSource(getAssetRoot() + "assets/shaders/alpha.vert", getAssetRoot() + "assets/shaders/alpha.frag");
}

ShaderSource PbrShader::GetSource() {
    return ShaderSource(getAssetRoot() + "assets/shaders/alpha.vert", getAssetRoot() + "assets/shaders/pbr.frag");
}

Model::Model(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) : mesh(std::move(mesh)), material(std::move(material)) {
    calculateBounds();
}

void Model::calculateBounds() {
    if (!mesh || mesh->vertices.empty()) {
        boundsCenter = glm::vec3(0.0f);
        boundsRadius = 0.0f;
        return;
    }

    glm::vec3 minPos = mesh->vertices[0].Position;
    glm::vec3 maxPos = mesh->vertices[0].Position;

    for (const auto& v : mesh->vertices) {
        minPos = glm::min(minPos, v.Position);
        maxPos = glm::max(maxPos, v.Position);
    }

    boundsCenter = (minPos + maxPos) * 0.5f;

    float maxDistSq = 0.0f;
    for (const auto& v : mesh->vertices) {
        glm::vec3 diff = v.Position - boundsCenter;
        float distSq = glm::dot(diff, diff);
        if (distSq > maxDistSq) {
            maxDistSq = distSq;
        }
    }
    boundsRadius = std::sqrt(maxDistSq);
}

glm::mat4 Transform::getWorldMatrix() const {
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    const glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
    return translation * rotationMatrix * scaling;
}

bool Object::isVisible(const Frustum& frustum) const {
    return isVisible(frustum, getWorldMatrix());
}

bool Object::isVisible(const Frustum& frustum, const glm::mat4& worldMatrix) const {
    if (!model)
        return false;

    if (model->boundsRadius <= 0.0f && model->mesh && !model->mesh->vertices.empty()) {
        model->calculateBounds();
    }

    glm::vec3 center = glm::vec3(worldMatrix * glm::vec4(model->boundsCenter, 1.0f));

    glm::vec3 absScale = glm::abs(scale);
    float maxScale = glm::max(glm::max(absScale.x, absScale.y), absScale.z);

    float radius = model->boundsRadius * maxScale;

    return frustum.intersectsSphere(center, radius);
}

glm::vec3 Transform::getFront() const {
    return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Transform::getRight() const {
    return glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Transform::getUp() const {
    return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
}
} // namespace knot
