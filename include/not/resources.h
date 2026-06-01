#pragma once

#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Mesh {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    
    unsigned int indexCount = 0;
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