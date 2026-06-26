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

#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

struct PointLight {
    vec4 position;    // 0 ~ 15 바이트
    vec4 color;       // 16 ~ 31 바이트 (rgb: 색상, a: 밝기)
    float radius;     // 32 ~ 35 바이트
    float constant;   // 36 ~ 39 바이트
    float linear;     // 40 ~ 43 바이트
    float quadratic;  // 44 ~ 47 바이트
}; // 명확하게 48바이트 블록으로 떨어짐

layout(std430, binding = 0) readonly buffer LightBuffer {
    PointLight pointLights[]; 
};

uniform int u_ActivePointLightCount;

struct Material {
    sampler2D albedoMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
};
uniform Material material;

uniform vec3 u_CameraPos;

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

vec3 calcPbrLight(vec3 N, vec3 V, vec3 L, vec3 lightColor, vec3 albedo, float metallic, float alphaRoughness, vec3 f0) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0001);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);

    float D = D_GGX(alphaRoughness, NoH, H);
    float V_func = V_SmithGGXCorrelated(alphaRoughness, NoV, NoL);
    vec3  F = F_Schlick(f0, 1.0, VoH);

    vec3 Fr = D * V_func * F;
    vec3 Fd = albedo * Fd_Lambert();

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic); 
    
    return (kD * Fd + Fr) * lightColor * NoL;
}

void main() {
    // 1. 주요 렌더링 벡터 계산
    vec3 N = normalize(Normal);
    vec3 V = normalize(u_CameraPos - FragPos);

    // 2. 텍스처 맵 샘플링 및 보정
    vec3 albedo     = texture(material.albedoMap, TexCoords).rgb;
    float metallic  = texture(material.metallicMap, TexCoords).r;
    float roughness = texture(material.roughnessMap, TexCoords).r;
    float ao        = texture(material.aoMap, TexCoords).r;

    float alphaRoughness = max(roughness * roughness, 0.002); 

    // 3. F0 계산
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    // 4. Directional Light 기여도 계산
    vec3 L_dir = normalize(-dirLight.direction);
    vec3 directLighting = calcPbrLight(N, V, L_dir, dirLight.diffuse, albedo, metallic, alphaRoughness, f0);
    vec3 ambient = dirLight.ambient * albedo * ao; 

    // 5. Point Light 기여도 계산
    for (int i = 0; i < u_ActivePointLightCount; ++i) {
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
        
        directLighting += calcPbrLight(N, V, L_point, lightColor, albedo, metallic, alphaRoughness, f0);
        ambient += (lightColorRaw * 0.05) * albedo * ao * attenuation;
    }
    vec3 finalColor = ambient + directLighting;

    // 6. Reinhard 톤매핑
    finalColor = finalColor / (finalColor + vec3(1.0));

    // 7. 감마 보정
    finalColor = pow(finalColor, vec3(1.0 / 2.2));  

    // 8. 최종 출력
    FragColor = vec4(finalColor, 1.0);
}