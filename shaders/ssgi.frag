#version 430 core

out vec4 IndirectLight;

in vec2 TexCoords;

uniform sampler2D gNormalDepth;
uniform sampler2D radianceMap;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform float radius;

#define PI 3.14159265359
const int RAY_COUNT = 8;
const int STEP_COUNT = 12;

vec3 reconstructViewPosition(vec2 uv, float depth) {
    vec4 clipPosition = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewRay = inverseProjection * clipPosition;
    viewRay.xyz /= max(abs(viewRay.w), 0.00001);
    return viewRay.xyz * (depth / max(-viewRay.z, 0.00001));
}

float interleavedGradientNoise(vec2 pixel) {
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

void main() {
    vec4 centerData = texture(gNormalDepth, TexCoords);
    float centerDepth = centerData.a;
    if (centerDepth <= 0.0) {
        IndirectLight = vec4(0.0);
        return;
    }

    vec3 origin = reconstructViewPosition(TexCoords, centerDepth);
    vec3 normal = normalize(centerData.rgb);

    vec3 helper = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 basis = mat3(tangent, bitangent, normal);

    float noise = interleavedGradientNoise(gl_FragCoord.xy);
    float rotation = noise * 2.0 * PI;

    vec3 radianceSum = vec3(0.0);
    for (int ray = 0; ray < RAY_COUNT; ++ray) {
        float u = (float(ray) + 0.5) / float(RAY_COUNT);
        float phi = 2.0 * PI * fract(float(ray) * 0.61803398875 + rotation / (2.0 * PI));
        float cosTheta = sqrt(1.0 - u);
        float sinTheta = sqrt(u);
        vec3 rayDirection = basis * vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

        float stepJitter = interleavedGradientNoise(gl_FragCoord.xy + float(ray) * 19.31);
        vec3 rayRadiance = vec3(0.0);

        for (int step = 1; step <= STEP_COUNT; ++step) {
            float t = (float(step) - stepJitter) / float(STEP_COUNT);
            float distanceAlongRay = radius * t;
            vec3 rayPosition = origin + normal * 0.05 + rayDirection * distanceAlongRay;

            vec4 projected = projection * vec4(rayPosition, 1.0);
            if (projected.w <= 0.0)
                break;

            vec2 sampleUV = projected.xy / projected.w * 0.5 + 0.5;
            if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
                break;

            vec4 hitData = texture(gNormalDepth, sampleUV);
            float sceneDepth = hitData.a;
            if (sceneDepth <= 0.0)
                continue;

            float rayDepth = -rayPosition.z;
            float depthDelta = rayDepth - sceneDepth;
            float thickness = max(0.15, distanceAlongRay * 0.25);

            if (depthDelta > 0.01 && depthDelta < thickness) {
                vec3 hitNormal = normalize(hitData.rgb);
                if (dot(hitNormal, -rayDirection) > 0.0) {
                    float distFade = 1.0 - smoothstep(0.6, 1.0, t);
                    vec2 edgeDist = smoothstep(vec2(0.0), vec2(0.08), sampleUV) * (1.0 - smoothstep(vec2(0.92), vec2(1.0), sampleUV));
                    float edgeFade = edgeDist.x * edgeDist.y;

                    rayRadiance = texture(radianceMap, sampleUV).rgb * distFade * edgeFade;
                }
                break;
            }
        }

        radianceSum += rayRadiance;
    }

    IndirectLight = vec4(radianceSum / float(RAY_COUNT), 1.0);
}