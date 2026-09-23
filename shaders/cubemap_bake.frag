#version 330 core
out vec4 fragColor;
in vec3 localPos;
uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
void main() {
    vec3 v = normalize(localPos);
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y)) * invAtan + 0.5;
    fragColor = vec4(texture(equirectangularMap, uv).rgb, 1.0);
}