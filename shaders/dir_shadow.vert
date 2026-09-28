#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 4) in mat4 instanceModel;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform bool isInstanced;

void main() {
    mat4 modelMatrix = isInstanced ? instanceModel : model;
    gl_Position = lightSpaceMatrix * modelMatrix * vec4(aPos, 1.0);
}
