#version 430 core

out float AmbientOcclusion;

in vec2 TexCoords;

uniform sampler2D gNormalDepth;
uniform sampler2D noiseTexture;
uniform vec3 samples[32];
uniform int sampleCount;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform vec2 screenSize;

const float SAMPLE_RADIUS = 0.6;
const float DEPTH_BIAS = 0.025;

vec3 reconstructViewPosition(vec2 uv, float depth) {
    vec4 clipPosition = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay = inverseProjection * clipPosition;
    viewRay.xyz /= max(abs(viewRay.w), 0.00001);
    return viewRay.xyz * (depth / max(-viewRay.z, 0.00001));
}

void main() {
    vec4 packedData = texture(gNormalDepth, TexCoords);
    float depth = packedData.a;
    if (depth <= 0.0) {
        AmbientOcclusion = 1.0;
        return;
    }

    vec3 position = reconstructViewPosition(TexCoords, depth);
    vec3 normal = normalize(packedData.rgb);
    vec3 randomVector = normalize(texture(noiseTexture, TexCoords * screenSize / 4.0).xyz * 2.0 - 1.0);
    vec3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        vec3 samplePosition = position + tbn * samples[i] * SAMPLE_RADIUS;
        vec4 projected = projection * vec4(samplePosition, 1.0);
        projected.xyz /= projected.w;
        vec2 sampleUV = projected.xy * 0.5 + 0.5;

        if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0)))) {
            continue;
        }

        float sampleDepth = texture(gNormalDepth, sampleUV).a;
        if (sampleDepth <= 0.0) {
            continue;
        }

        float rangeWeight = smoothstep(0.0, 1.0, SAMPLE_RADIUS / max(abs(depth - sampleDepth), 0.0001));
        float blocked = sampleDepth < -samplePosition.z - DEPTH_BIAS ? 1.0 : 0.0;
        occlusion += blocked * rangeWeight;
    }

    AmbientOcclusion = clamp(1.0 - occlusion / float(sampleCount), 0.0, 1.0);
}
