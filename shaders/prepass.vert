#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec3 aNormal;
layout(location = 4) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isInstanced;

out vec2 TexCoords;
out vec3 ViewNormal;
out float ViewDepth;

void main() {
    mat4 modelMatrix = isInstanced ? instanceModel : model;
    vec4 viewPosition = view * modelMatrix * vec4(aPos, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(view * modelMatrix)));

    TexCoords = aTexCoords;
    ViewNormal = normalize(normalMatrix * aNormal);
    ViewDepth = -viewPosition.z;
    gl_Position = projection * viewPosition;
}
