/*
 * Copyright (c) 2026 SinokaDev
 * * This file contains code derived from Google's Filament project.
 * Original code Copyright Google LLC.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#version 450
layout(location=0) out vec4 FragColor;
layout(location=0) in vec3 FragPos;
layout(location=1) in vec2 TexCoords;
layout(location=2) in vec3 Normal;
layout(location=3) in mat3 TBN;
layout(location=6) in vec4 LightSpaceFragPos;
layout(set=0,binding=0,std140) uniform SceneUniform {
    mat4 viewProjection;
    mat4 lightSpaceMatrix;
    mat4 skyViewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightDiffuse;
    vec4 lightAmbient;
    ivec4 counts;
} scene;
layout(push_constant) uniform MaterialUniform { vec4 albedo; vec4 factors; } params;

struct PointLight { vec4 position; vec4 color; float radius; float constant; float linear; float quadratic; };
layout(set=0,binding=1,std430) readonly buffer LightBuffer { PointLight pointLights[]; };
layout(set=1,binding=0) uniform sampler2D albedoMap;
layout(set=1,binding=1) uniform sampler2D metallicMap;
layout(set=1,binding=2) uniform sampler2D roughnessMap;
layout(set=1,binding=3) uniform sampler2D aoMap;
layout(set=1,binding=4) uniform sampler2D normalMap;
layout(set=1,binding=5) uniform samplerCube irradianceMap;
layout(set=1,binding=6) uniform samplerCube prefilterMap;
layout(set=1,binding=7) uniform sampler2D brdfLUT;
layout(set=1,binding=8) uniform sampler2D shadowMap;
#define PI 3.14159265359

float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

float PREVENT_DIV0(float num, float den, float alsh) {
    return num / max(den, alsh);
}

float D_GGX(float alpha, float NoH, const vec3 h) {
    float oneMinusNoHSquared = 1.0 - NoH * NoH;

    float a = NoH * alpha;
    float k = min(alpha / (oneMinusNoHSquared + a * a), 453.5);
    float d = k * (k * (1.0 / PI));
    return d;
}

float V_SmithGGXCorrelated(float alpha, float NoV, float NoL) {
    float a2 = alpha;
    float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    float v = PREVENT_DIV0(0.5, lambdaV + lambdaL, 0.0000077);
    return v;
}

vec3 F_Schlick(const vec3 f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float Fd_Lambert() {
    return 1.0 / PI;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 calcPbrLight(vec3 N, vec3 V, vec3 L, vec3 lightColor, vec3 albedo, float metallic, float alphaRoughness, vec3 f0, float shadow) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0001);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);

    float D = D_GGX(alphaRoughness, NoH, H);
    float V_func = V_SmithGGXCorrelated(alphaRoughness, NoV, NoL);
    vec3 F = F_Schlick(f0, 1.0, VoH);

    vec3 Fr = D * V_func * F;
    vec3 Fd = albedo * Fd_Lambert();

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    return ((kD * Fd + Fr) * lightColor * NoL) * (1.0 - shadow);
}

float calcShadow(vec4 lightSpaceFragPos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (scene.counts.y == 0 || projCoords.z > 1.0 || projCoords.z < 0.0 || any(lessThan(projCoords.xy,vec2(0))) || any(greaterThan(projCoords.xy,vec2(1)))) {
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
    vec3 V = normalize(scene.cameraPosition.xyz - FragPos);

    // Texture Map
    vec3 albedo = texture(albedoMap, TexCoords).rgb * params.albedo.rgb;
    float metallic = texture(metallicMap, TexCoords).r * params.factors.x;
    float roughness = texture(roughnessMap, TexCoords).r * params.factors.y;
    float ao = texture(aoMap, TexCoords).r * params.factors.z;

    // Normal Map
    vec3 sampledNormal = texture(normalMap, TexCoords).rgb;
    vec3 N = normalize(sampledNormal * 2.0 - 1.0);
    N = normalize(TBN * N);

    float alphaRoughness = max(roughness * roughness, 0.002);

    // F0
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    // Directional Light
    vec3 L_dir = normalize(-scene.lightDirection.xyz);
    vec3 directLighting = calcPbrLight(N, V, L_dir, scene.lightDiffuse.xyz, albedo, metallic, alphaRoughness, f0, calcShadow(LightSpaceFragPos, N, L_dir));

    // Fresnel
    float NoV = max(dot(N, V), 0.0);
    vec3 kS = fresnelSchlickRoughness(NoV, f0, roughness);
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    // Diffuse IBL
    vec3 irradiance = texture(irradianceMap, normalize(N)).rgb;
    vec3 diffuse = irradiance * albedo;

    // Specular IBL
    vec3 R = reflect(-V, N);
    float mipLevel = roughness * 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, mipLevel).rgb;
    vec2 brdf = texture(brdfLUT, vec2(NoV, roughness)).rg;
    vec3 specular = prefilteredColor * (kS * brdf.x + brdf.y);

    // Ambient
    vec3 ambient = (kD * diffuse + specular + scene.lightAmbient.rgb * albedo) * ao;

    // Point Light
    for (int i = 0; i < scene.counts.x; ++i) {
        vec3 lightPos = pointLights[i].position.xyz;
        vec3 lightColorRaw = pointLights[i].color.rgb;
        float brightness = pointLights[i].color.a;

        vec3 L_point = normalize(lightPos - FragPos);
        float dist = length(lightPos - FragPos);

        // 기본 역제곱 감쇄
        float attenuation = 1.0 / (dist * dist + 0.01);

        // 부드러운 감쇄 창 함수
        float factor = dist / pointLights[i].radius;
        float windowing = clamp(1.0 - factor * factor * factor * factor, 0.0, 1.0);
        windowing *= windowing;
        attenuation *= windowing;

        // 최종 광도 계산
        vec3 lightColor = lightColorRaw * brightness * attenuation;

        directLighting += calcPbrLight(N, V, L_point, lightColor, albedo, metallic, alphaRoughness, f0, 0.0);
    }
    vec3 finalColor = ambient + directLighting;

    // Reinhard 톤매핑
    finalColor = finalColor / (finalColor + vec3(1.0));

    // 감마 보정
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    FragColor = vec4(finalColor, 1.0);
}