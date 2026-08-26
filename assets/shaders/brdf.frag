#version 430 core
out vec2 FragColor;
in vec2 TexCoords;

const float PI = 3.14159265359;

// 반톤(Low-discrepancy) 시퀀스 생성을 위한 반전 비트 함수
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// GGX 중요도 샘플링
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    // 구면 좌표계를 직교 좌표계로
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // 접선 공간(Tanget Space)에서 월드 공간으로
    vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

// Smith GGX 가시성 함수
float GeometrySchlickGGX(float NoV, float roughness) {
    // IBL용 k 계산 방식 (직접광 k=(r+1)^2 / 8 과 다름)
    float k = (roughness * roughness) / 2.0;
    return NoV / (NoV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NoV, roughness);
    float ggx1 = GeometrySchlickGGX(NoL, roughness);
    
    return ggx1 * ggx2;
}

// BRDF 적분 핵심 함수
vec2 IntegrateBRDF(float NoV, float roughness) {
    vec3 V;
    V.x = sqrt(1.0 - NoV * NoV);
    V.y = 0.0;
    V.z = NoV;
    
    float A = 0.0;
    float B = 0.0;
    
    vec3 N = vec3(0.0, 0.0, 1.0);
    
    const uint SAMPLE_COUNT = 1024u; // 샘플 수 (1024회면 매우 정밀함)
    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NoL = max(L.z, 0.0);
        float NoH = max(H.z, 0.0);
        float VoH = max(dot(V, H), 0.0);
        
        if(NoL > 0.0) {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VoH) / (NoH * NoV);
            float Fc = pow(1.0 - VoH, 5.0);
            
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main() {
    // TexCoords.x = NdotV (0~1), TexCoords.y = Roughness (0~1)
    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);
    FragColor = integratedBRDF; // R=Scale, G=Bias
}