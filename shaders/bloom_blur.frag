#version 330 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_InputTex;
uniform vec2 u_Resolution;
uniform bool u_Horizontal;

void main() {
    vec2 texel = 1.0 / u_Resolution;
    vec2 axis = u_Horizontal ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    vec3 result = texture(u_InputTex, v_TexCoord).rgb * 0.227027;
    result += texture(u_InputTex, v_TexCoord + axis).rgb * 0.1945946;
    result += texture(u_InputTex, v_TexCoord - axis).rgb * 0.1945946;
    result += texture(u_InputTex, v_TexCoord + axis * 2.0).rgb * 0.1216216;
    result += texture(u_InputTex, v_TexCoord - axis * 2.0).rgb * 0.1216216;
    result += texture(u_InputTex, v_TexCoord + axis * 3.0).rgb * 0.054054;
    result += texture(u_InputTex, v_TexCoord - axis * 3.0).rgb * 0.054054;
    result += texture(u_InputTex, v_TexCoord + axis * 4.0).rgb * 0.016216;
    result += texture(u_InputTex, v_TexCoord - axis * 4.0).rgb * 0.016216;

    FragColor = vec4(result, 1.0);
}
