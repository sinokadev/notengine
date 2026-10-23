#version 430 core

out float BlurredAO;

in vec2 TexCoords;

uniform sampler2D ssaoInput;
uniform sampler2D gNormalDepth;
uniform vec2 screenSize;

void main() {
    vec2 texel = 1.0 / screenSize;
    vec4 centerData = texture(gNormalDepth, TexCoords);
    float centerDepth = centerData.a;
    vec3 centerNormal = normalize(centerData.rgb);
    float weightedAO = 0.0;
    float totalWeight = 0.0;

    for (int y = -2; y <= 1; ++y) {
        for (int x = -2; x <= 1; ++x) {
            vec2 uv = TexCoords + vec2(x, y) * texel;
            vec4 sampleData = texture(gNormalDepth, uv);
            float depthWeight = exp(-abs(sampleData.a - centerDepth) * 8.0);
            float normalWeight = pow(max(dot(normalize(sampleData.rgb), centerNormal), 0.0), 8.0);
            float weight = depthWeight * normalWeight;
            weightedAO += texture(ssaoInput, uv).r * weight;
            totalWeight += weight;
        }
    }

    BlurredAO = weightedAO / max(totalWeight, 0.0001);
}
