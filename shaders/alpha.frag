#version 430 core
out vec4 FragColor;

// 0: direct draw, 1: opaque fragments, 2: translucent fragments.
uniform int alphaPass;

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
    if (FragColor.a <= 0.0 || (alphaPass == 1 && FragColor.a < 1.0) || (alphaPass == 2 && FragColor.a >= 1.0)) {
        discard;
    }
}
