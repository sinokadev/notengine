#version 430 core
out vec4 FragColor;

// 0: direct draw, 1: opaque fragments, 2: translucent fragments.
uniform int alphaPass;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec4 LightSpaceFragPos;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

struct PointLight {
    vec4 position;
    vec4 color;
    float radius;
    float constant;
    float linear;
    float quadratic;
};

layout(std430, binding = 0) readonly buffer LightBuffer {
    PointLight pointLights[];
};

uniform int activePointLightCount;

struct Material {
    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
};
uniform Material material;

uniform vec3 cameraPos;
uniform float maxReflectionLOD;
uniform float ambientIntensity;
uniform sampler2D brdfLUT;
uniform sampler2D ssaoMap;
uniform bool ssaoEnabled;
uniform sampler2D shadowMap;
uniform samplerCubeArray pointShadowMap;
uniform int pointShadowCount;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;

#define PI 3.14159265359

// ACES Tone Mapping for better contrast and saturation in highlights
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float computeMicrofacetDistribution(vec3 surfaceNormal, vec3 halfVector, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(surfaceNormal, halfVector), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.000001);
}

float computeGeometryAttenuationFactor(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, 0.000001);
}

float computeCombinedShadowMask(float NdotV, float NdotL, float roughness) {
    float geoView = computeGeometryAttenuationFactor(NdotV, roughness);
    float geoLight = computeGeometryAttenuationFactor(NdotL, roughness);
    return geoView * geoLight;
}

vec3 computeFresnelResponse(float cosTheta, vec3 baseReflectivity) {
    return baseReflectivity + (1.0 - baseReflectivity) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 computeFresnelWithRoughness(float cosTheta, vec3 baseReflectivity, float roughness) {
    return baseReflectivity + (max(vec3(1.0 - roughness), baseReflectivity) - baseReflectivity) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 calcPbrLight(vec3 N, vec3 V, vec3 L, vec3 lightColor, vec3 albedo, float metalness, float roughness, vec3 F0, float shadowFactor) {
    if (shadowFactor >= 1.0)
        return vec3(0.0);

    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0)
        return vec3(0.0);

    float NdotV = max(dot(N, V), 0.0001);
    vec3 H = normalize(V + L);
    float HdotV = max(dot(H, V), 0.0);

    float D = computeMicrofacetDistribution(N, H, roughness);
    float G = computeCombinedShadowMask(NdotV, NdotL, roughness);
    vec3 F = computeFresnelResponse(HdotV, F0);

    vec3 specNumerator = D * G * F;
    float specDenominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = specNumerator / specDenominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness);

    return (kD * (albedo / PI) + specular) * lightColor * NdotL * (1.0 - shadowFactor);
}

float calcPointShadow(int index, vec3 normal, vec3 lightDir) {
    int shadowLayer = int(pointLights[index].position.w);
    if (shadowLayer < 0 || shadowLayer >= pointShadowCount)
        return 0.0;

    vec3 fromLight = FragPos - pointLights[index].position.xyz;
    float distanceToLight = length(fromLight);
    float farPlane = max(0.2, pointLights[index].radius);

    if (distanceToLight <= 0.000001 || distanceToLight >= farPlane)
        return 0.0;

    vec3 direction = fromLight / distanceToLight;
    vec3 up = abs(direction.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, direction));
    vec3 bitangent = cross(direction, tangent);
    float texelSize = 2.0 / float(textureSize(pointShadowMap, 0).x);
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = max(0.02 * (1.0 - cosTheta), 0.003) + distanceToLight * texelSize;

    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec3 sampleDirection = direction + (tangent * float(x) + bitangent * float(y)) * texelSize;
            float closestDepth = texture(pointShadowMap, vec4(sampleDirection, float(shadowLayer))).r * farPlane;
            shadow += distanceToLight - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

float calcShadow(vec4 lightSpaceFragPos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;
    float normalLightAlignment = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = max(0.0015 * (1.0 - normalLightAlignment), 0.0002);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main() {
    // Alpha Clipping Pass
    vec4 albedoSample = texture(material.albedoMap, TexCoords);
    if (albedoSample.a <= 0.0 || (alphaPass == 1 && albedoSample.a < 1.0) || (alphaPass == 2 && albedoSample.a >= 1.0)) {
        discard;
    }

    vec3 V = normalize(cameraPos - FragPos);

    // Texture Sampling
    vec3 albedo = albedoSample.rgb;
    float metallic = texture(material.metallicMap, TexCoords).r;
    float roughness = texture(material.roughnessMap, TexCoords).r;
    float ao = texture(material.aoMap, TexCoords).r;

    // Normal Mapping
    vec3 normalMap = texture(material.normalMap, TexCoords).rgb;
    vec3 N = normalize(TBN * (normalMap * 2.0 - 1.0));
    vec3 geometricNormal = normalize(Normal);

    // Base Reflectivity (F0)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Directional Light
    vec3 L_dir = normalize(-dirLight.direction);
    float dirShadow = calcShadow(LightSpaceFragPos, geometricNormal, L_dir);
    vec3 directLighting = calcPbrLight(N, V, L_dir, dirLight.diffuse, albedo, metallic, roughness, F0, dirShadow);

    // Point Lights with Early Out Optimization
    for (int i = 0; i < activePointLightCount; ++i) {
        vec3 lightPos = pointLights[i].position.xyz;
        vec3 L_point = lightPos - FragPos;
        float dist = length(L_point);
        float radius = pointLights[i].radius;

        if (dist >= radius)
            continue; // Early exit outside influence radius

        L_point /= dist; // Normalize

        float NdotL = dot(N, L_point);
        if (NdotL <= 0.0)
            continue; // Skip backside lighting & shadow map reads

        // Attenuation calculation
        float attenuation = 1.0 / (dist * dist + 0.01);
        float factor = dist / radius;
        float windowing = clamp(1.0 - factor * factor * factor * factor, 0.0, 1.0);
        attenuation *= windowing * windowing;

        vec3 lightColor = pointLights[i].color.rgb * pointLights[i].color.a * attenuation;

        // Compute shadow only when light intensity actually matters
        float pointShadow = 0.0;
        if (attenuation > 0.001) {
            pointShadow = calcPointShadow(i, geometricNormal, L_point);
        }

        directLighting += calcPbrLight(N, V, L_point, lightColor, albedo, metallic, roughness, F0, pointShadow);
    }

    // IBL Ambient Lighting
    float NoV = max(dot(N, V), 0.0);
    vec3 F_ibl = computeFresnelWithRoughness(NoV, F0, roughness);
    vec3 kD_ibl = (vec3(1.0) - F_ibl) * (1.0 - metallic);

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuseIBL = irradiance * albedo;

    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * maxReflectionLOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(NoV, roughness)).rg;

    // Corrected Specular IBL calculation using Standard BRDF Integration
    vec3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);

    float ssao = ssaoEnabled ? texture(ssaoMap, gl_FragCoord.xy / vec2(textureSize(ssaoMap, 0))).r : 1.0;
    vec3 ambient = (kD_ibl * diffuseIBL + specularIBL) * ao * ambientIntensity * ssao;

    vec3 finalColor = ambient + directLighting;

    // ACES Tone Mapping & Gamma Correction
    finalColor = ACESFilm(finalColor);
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    FragColor = vec4(finalColor, albedoSample.a);
}
