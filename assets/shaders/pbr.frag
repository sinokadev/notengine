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

uniform int u_ActivePointLightCount;

struct Material {
    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
};
uniform Material material;

uniform vec3 u_CameraPos;

uniform samplerCube irradianceMap;

uniform float u_AmbientIntensity;

uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;
uniform float       u_MaxReflectionLOD;

uniform sampler2D shadowMap;

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
    vec3  F = F_Schlick(f0, 1.0, VoH);

    vec3 Fr = D * V_func * F;
    vec3 Fd = albedo * Fd_Lambert();

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic); 
    
    return ((kD * Fd + Fr) * lightColor * NoL) * (1.0 - shadow);
}

float calcShadow(vec4 lightSpaceFragPos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;

    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0) {
        return 0.0;
    }

    float closestDepth = texture(shadowMap, projCoords.xy).r; 

    float currentDepth = projCoords.z;

    vec3 N = normalize(normal);

    vec3 L = normalize(lightDir);

    float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005); 

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
}
void main() {
    vec3 V = normalize(u_CameraPos - FragPos);

    // Texture Map
    vec3 albedo     = texture(material.albedoMap, TexCoords).rgb;
    float metallic  = texture(material.metallicMap, TexCoords).r;
    float roughness = texture(material.roughnessMap, TexCoords).r;
    float ao        = texture(material.aoMap, TexCoords).r;

    // Normal Map
    vec3 normalMap = texture(material.normalMap, TexCoords).rgb;
    vec3 N = normalize(normalMap * 2.0 - 1.0);
    N = normalize(TBN * N);

    float alphaRoughness = max(roughness * roughness, 0.002); 

    // F0
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    // Directional Light
    vec3 L_dir = normalize(-dirLight.direction);
    vec3 directLighting = calcPbrLight(N, V, L_dir, dirLight.diffuse, albedo, metallic, alphaRoughness, f0, calcShadow(LightSpaceFragPos, N, L_dir));

    // Fresnel
    float NoV = max(dot(N, V), 0.0);
    vec3 kS = fresnelSchlickRoughness(NoV, f0, roughness); 
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic); 

    // Diffuse IBL
    vec3 irradiance = texture(irradianceMap, normalize(N)).rgb;
    vec3 diffuse = irradiance * albedo;

    // Specular IBL
    vec3 R = reflect(-V, N);
    float mipLevel = roughness * u_MaxReflectionLOD;
    vec3 prefilteredColor = textureLod(prefilterMap, R, mipLevel).rgb;
    vec2 brdf = texture(brdfLUT, vec2(NoV, roughness)).rg;
    vec3 specular = prefilteredColor * (kS * brdf.x + brdf.y);

    // Ambient
    vec3 ambient = (kD * diffuse + specular) * ao * u_AmbientIntensity;

    // Point Light
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
        
        directLighting += calcPbrLight(N, V, L_point, lightColor, albedo, metallic, alphaRoughness, f0, 0.0);
    }
    vec3 finalColor = ambient + directLighting;

    // Reinhard 톤매핑
    finalColor = finalColor / (finalColor + vec3(1.0));

    // 감마 보정
    finalColor = pow(finalColor, vec3(1.0 / 2.2));  

    FragColor = vec4(finalColor, 1.0);
}