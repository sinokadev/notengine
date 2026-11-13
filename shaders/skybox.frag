#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float exposure;
uniform bool captureRadiance;

void main() {
    vec3 envColor = texture(skybox, TexCoords).rgb;

    if (captureRadiance) {
        FragColor = vec4(envColor, 1.0);
        return;
    }

    envColor *= exposure;

    envColor = envColor / (envColor + vec3(1.0));

    envColor = pow(envColor, vec3(1.0 / 2.2));

    FragColor = vec4(envColor, 1.0);
}