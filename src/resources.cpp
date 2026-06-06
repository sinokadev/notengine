#include <knot/resources.h>

#include <sstream>
#include <iostream>
#include <fstream>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace knot {

    Shader::Shader(std::string vertex_path, std::string fragment_path) {
        std::string vertex_code;
        std::string fragment_code;
        std::ifstream vertex_file;
        std::ifstream fragment_file;

        vertex_file.open(vertex_path);

        if (!vertex_file.is_open()) {
            std::cerr << "Failed to open vertex shader file: " << vertex_path << std::endl;
            return;
        }

        fragment_file.open(fragment_path);

        if (!fragment_file.is_open()) {
            std::cerr << "Failed to open fragment shader file: " << fragment_path << std::endl;
            return;
        }

        std::stringstream vertex_stream, fragment_stream;

        vertex_stream << vertex_file.rdbuf();
        fragment_stream << fragment_file.rdbuf();
        
        vertex_code = vertex_stream.str();
        fragment_code = fragment_stream.str();
        
        vertex_file.close();
        fragment_file.close();

        const char *vertex_string = vertex_code.c_str();
        const char *fragment_string = fragment_code.c_str();

        // Compile vertex shader
        unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &vertex_string, NULL);
        glCompileShader(vertex_shader);

        int success;
        char log[512];
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex_shader, 512, NULL, log);
            std::cerr << "Failed to compile vertex shader\n" << log << std::endl;
        }

        // Compile fragment shader
        unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &fragment_string, NULL);
        glCompileShader(fragment_shader);

        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment_shader, 512, NULL, log);
            std::cerr << "Failed to compile fragment shader\n" << log << std::endl;
        }

        // Link shader
        shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);

        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_program, 512, NULL, log);
            std::cerr << "Failed to link shaders\n" << log << std::endl;
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    void Shader::use() { 
        glUseProgram(shader_program); 
    }

    void Shader::set(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(shader_program, name.c_str()), (int)value);
    }

    void Shader::set(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(shader_program, name.c_str()), value);
    }

    void Shader::set(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(shader_program, name.c_str()), value);
    }

    void Shader::set(const std::string &name, const glm::vec2 &value) const {
        glUniform2fv(glGetUniformLocation(shader_program, name.c_str()), 1, &value[0]);
    }


    void Shader::set(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(shader_program, name.c_str()), 1, &value[0]);
    }

    void Shader::set(const std::string &name, const glm::mat4 &value) const {
        glUniformMatrix4fv(glGetUniformLocation(shader_program, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }

    void Shader::set(int location, bool value) const {
        glUniform1i(location, value);
    }

    void Shader::set(int location, int value) const {
        glUniform1i(location, value);
    }

    void Shader::set(int location, float value) const {
        glUniform1f(location, value);
    }

    void Shader::set(int location, const glm::vec2 &value) const {
        glUniform2fv(location, 1, &value[0]);
    }

    void Shader::set(int location, const glm::vec3 &value) const {
        glUniform3fv(location, 1, &value[0]);
    }

    void Shader::set(int location, const glm::mat4 &value) const {
        glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    }

    unsigned int Shader::get_id() const {
        return shader_program;
    }

    void Shader::destroy() {
        glDeleteProgram(shader_program);
    }

    Mesh::~Mesh() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo); 
    }

    glm::mat4 Object::getWorldMatrix() const {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
        
        glm::mat4 rot = glm::mat4_cast(rotation);
        
        glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);

        return trans * rot * sca;
    }

}