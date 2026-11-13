#version 430 core

out vec4 BlurredIndirectLight;

in vec2 TexCoords;

uniform sampler2D ssgiInput;
uniform sampler2D gNormalDepth;
uniform vec2 screenSize;

void main() {
    vec4 centerData = texture(gNormalDepth, TexCoords);
    if (centerData.a <= 0.0) {
        BlurredIndirectLight = vec4(0.0);
        return;
    }

    vec3 centerNormal = normalize(centerData.rgb);
    vec2 texel = 1.0 / screenSize;
    vec3 weightedLight = vec3(0.0);
    float totalWeight = 0.0;

    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            vec2 offset = vec2(x, y);
            vec2 sampleUV = TexCoords + offset * texel;
            if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
                continue;

            vec4 sampleData = texture(gNormalDepth, sampleUV);
            if (sampleData.a <= 0.0)
                continue;

            float depthWeight = exp(-abs(sampleData.a - centerData.a) * 12.0);
            float normalWeight = pow(max(dot(normalize(sampleData.rgb), centerNormal), 0.0), 16.0);
            float spatialWeight = exp(-dot(offset, offset) * 0.25);
            float weight = depthWeight * normalWeight * spatialWeight;
            weightedLight += texture(ssgiInput, sampleUV).rgb * weight;
            totalWeight += weight;
        }
    }

    BlurredIndirectLight = vec4(weightedLight / max(totalWeight, 0.0001), 1.0);
}
