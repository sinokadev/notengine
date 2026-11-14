#version 430 core

in vec3 v_Normal;
out vec4 FragColor;

uniform vec3 u_EmissionColor;

void main() {
    vec3 lightDirection = normalize(vec3(-0.45, 0.8, 0.55));
    float surfaceLight = 0.35 + 0.65 * max(dot(normalize(v_Normal), lightDirection), 0.0);
    FragColor = vec4(u_EmissionColor * surfaceLight, 1.0);
}
