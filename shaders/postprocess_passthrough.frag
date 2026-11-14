#version 330 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_InputTex;

void main() {
    FragColor = texture(u_InputTex, v_TexCoord);
}
