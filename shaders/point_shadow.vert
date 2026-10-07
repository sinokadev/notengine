#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 4) in mat4 aInstanceMatrix;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform bool isInstanced;
out vec4 FragPos;

void main() {
    mat4 finalModel = isInstanced ? aInstanceMatrix : model;
    FragPos = finalModel * vec4(aPos, 1.0);
    gl_Position = lightSpaceMatrix * FragPos;
}
