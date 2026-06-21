#include <knot/resources.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

ShaderSource::ShaderSource(std::string v, std::string f) : vertexPath(std::move(v)), fragmentPath(std::move(f)) {
    vertexSourceCode = readFile(vertexPath);
    fragmentSourceCode = readFile(fragmentPath);
}

bool ShaderSource::isValid() const {
    return !vertexSourceCode.empty() && !fragmentSourceCode.empty();
}

std::string ShaderSource::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Error] Failed to open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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
        glDeleteProgram(shaderProgram);
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

ShaderSource PongShader::GetSource() {
    return ShaderSource(getAssetRoot() + "assets/shaders/alpha.vert", getAssetRoot() + "assets/shaders/pong.frag");
}

ShaderSource AlphaShader::GetSource() {
    return ShaderSource(getAssetRoot() + "assets/shaders/alpha.vert", getAssetRoot() + "assets/shaders/alpha.frag");
}

Mesh::~Mesh() {
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

glm::mat4 Object::getWorldMatrix() const {
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    const glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
    return translation * rotationMatrix * scaling;
}

} // namespace knot
