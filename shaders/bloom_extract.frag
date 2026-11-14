#version 330 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_InputTex;
uniform float u_Threshold;

void main() {
    vec3 color = texture(u_InputTex, v_TexCoord).rgb;
    float brightness = max(max(color.r, color.g), color.b);
    FragColor = vec4(brightness > u_Threshold ? color : vec3(0.0), 1.0);
}
