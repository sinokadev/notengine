#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

struct Material {
    vec3 color;
};
uniform Material material;

void main() {
    FragColor = vec4(material.color, 1.0);
}