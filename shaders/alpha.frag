#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

struct Material {
    sampler2D diffuse;
};
uniform Material material;

void main() {
    FragColor = texture(material.diffuse, TexCoords);
}