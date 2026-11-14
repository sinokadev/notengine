#version 330 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_InputTex;
uniform sampler2D u_SceneColorTex;
uniform float u_BloomIntensity;

void main() {
    vec4 scene = texture(u_SceneColorTex, v_TexCoord);
    vec3 bloomColor = texture(u_InputTex, v_TexCoord).rgb;
    FragColor = vec4(scene.rgb + bloomColor * u_BloomIntensity, scene.a);
}
