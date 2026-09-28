#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec4 LightSpaceFragPos;

// structs and uniforms

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
uniform sampler2D shadowMap;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;

#define PI 3.14159265359

float computeMicrofacetDistribution(vec3 surfaceNormal, vec3 halfVector, float glossiness) {
    float roughnessSquared = glossiness * glossiness;
    float roughnessFourth = roughnessSquared * roughnessSquared;
    float normalDotHalf = max(dot(surfaceNormal, halfVector), 0.0);
    float normalDotHalfSq = normalDotHalf * normalDotHalf;

    float numerator = roughnessFourth;
    float denominator = (normalDotHalfSq * (roughnessFourth - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return numerator / max(denominator, 0.000001);
}

float computeGeometryAttenuationFactor(float angleDot, float glossiness) {
    float adjustedRoughness = (glossiness + 1.0);
    float kParam = (adjustedRoughness * adjustedRoughness) / 8.0;

    float numerator = angleDot;
    float denominator = angleDot * (1.0 - kParam) + kParam;

    return numerator / max(denominator, 0.000001);
}

float computeCombinedShadowMask(vec3 surfaceNormal, vec3 viewDir, vec3 lightDir, float glossiness) {
    float viewCos = max(dot(surfaceNormal, viewDir), 0.0);
    float lightCos = max(dot(surfaceNormal, lightDir), 0.0);
    float geoView = computeGeometryAttenuationFactor(viewCos, glossiness);
    float geoLight = computeGeometryAttenuationFactor(lightCos, glossiness);

    return geoView * geoLight;
}

vec3 computeFresnelResponse(float cosTheta, vec3 baseReflectivity) {
    return baseReflectivity + (1.0 - baseReflectivity) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 computeFresnelWithRoughness(float cosTheta, vec3 baseReflectivity, float glossiness) {
    return baseReflectivity + (max(vec3(1.0 - glossiness), baseReflectivity) - baseReflectivity) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 evaluateSurfaceLighting(vec3 surfaceNormal, vec3 viewDir, vec3 lightDir, vec3 lightColor, vec3 surfaceAlbedo, float metalness, float glossiness,
                             vec3 baseReflectivity, float shadowFactor) {
    vec3 halfVector = normalize(viewDir + lightDir);
    float normalDotView = max(dot(surfaceNormal, viewDir), 0.0001);
    float normalDotLight = max(dot(surfaceNormal, lightDir), 0.0);
    float halfDotView = max(dot(halfVector, viewDir), 0.0);

    float microfacetDist = computeMicrofacetDistribution(surfaceNormal, halfVector, glossiness);
    float geometryMask = computeCombinedShadowMask(surfaceNormal, viewDir, lightDir, glossiness);
    vec3 fresnelTerm = computeFresnelResponse(halfDotView, baseReflectivity);

    vec3 specularNumerator = microfacetDist * geometryMask * fresnelTerm;
    float specularDenominator = 4.0 * normalDotView * normalDotLight + 0.0001;
    vec3 specularReflectance = specularNumerator / specularDenominator;

    vec3 specularWeight = fresnelTerm;
    vec3 diffuseWeight = vec3(1.0) - specularWeight;
    diffuseWeight *= (1.0 - metalness);

    return (diffuseWeight * (surfaceAlbedo / PI) + specularReflectance) * lightColor * normalDotLight * (1.0 - shadowFactor);
}

float calcShadow(vec4 lightSpaceFragPos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) {
        return 0.0;
    }

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);

    float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main() {
    vec3 V = normalize(cameraPos - FragPos);

    // Texture Maps
    vec3 albedo = texture(material.albedoMap, TexCoords).rgb;
    float metallic = texture(material.metallicMap, TexCoords).r;
    float roughness = texture(material.roughnessMap, TexCoords).r;
    float ao = texture(material.aoMap, TexCoords).r;

    // Normal Mapping
    vec3 normalMap = texture(material.normalMap, TexCoords).rgb;
    vec3 N = normalize(normalMap * 2.0 - 1.0);
    N = normalize(TBN * N);

    // Base Reflectivity (F0)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Directional Light
    vec3 L_dir = normalize(-dirLight.direction);
    vec3 directLighting =
        evaluateSurfaceLighting(N, V, L_dir, dirLight.diffuse, albedo, metallic, roughness, F0, calcShadow(LightSpaceFragPos, N, L_dir));

    // Fresnel for IBL
    float NoV = max(dot(N, V), 0.0);
    vec3 kS = computeFresnelWithRoughness(NoV, F0, roughness);
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    // Diffuse IBL
    vec3 irradiance = texture(irradianceMap, normalize(N)).rgb;
    vec3 diffuse = irradiance * albedo;

    // Specular IBL
    vec3 R = reflect(-V, N);
    float mipLevel = roughness * maxReflectionLOD;
    vec3 prefilteredColor = textureLod(prefilterMap, R, mipLevel).rgb;
    vec2 brdf = texture(brdfLUT, vec2(NoV, roughness)).rg;
    vec3 specular = prefilteredColor * (kS * brdf.x + brdf.y);

    // Ambient Lighting
    vec3 ambient = (kD * diffuse + specular) * ao * ambientIntensity;

    // Point Lights
    for (int i = 0; i < activePointLightCount; ++i) {
        vec3 lightPos = pointLights[i].position.xyz;
        vec3 lightColorRaw = pointLights[i].color.rgb;
        float brightness = pointLights[i].color.a;

        vec3 L_point = normalize(lightPos - FragPos);
        float dist = length(lightPos - FragPos);

        // Basic Inverse-Square Attenuation
        float attenuation = 1.0 / (dist * dist + 0.01);

        // Smooth Windowing Attenuation Function
        float factor = dist / pointLights[i].radius;
        float windowing = clamp(1.0 - factor * factor * factor * factor, 0.0, 1.0);
        windowing *= windowing;
        attenuation *= windowing;

        // Final Light Intensity Calculation
        vec3 lightColor = lightColorRaw * brightness * attenuation;

        directLighting += evaluateSurfaceLighting(N, V, L_point, lightColor, albedo, metallic, roughness, F0, 0.0);
    }

    vec3 finalColor = ambient + directLighting;

    // Reinhard Tone Mapping
    finalColor = finalColor / (finalColor + vec3(1.0));

    // Gamma Correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    FragColor = vec4(finalColor, 1.0);
}