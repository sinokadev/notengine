#version 430 core

layout(location = 0) out vec4 NormalDepth;

in vec2 TexCoords;
in vec3 ViewNormal;
in float ViewDepth;

struct Material {
    sampler2D albedoMap;
};
uniform Material material;
uniform int alphaPass;

void main() {
    float alpha = texture(material.albedoMap, TexCoords).a;
    if (alpha <= 0.0 || (alphaPass == 1 && alpha < 1.0)) {
        discard;
    }

    NormalDepth = vec4(normalize(ViewNormal), ViewDepth);
}
