#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;

// Instance transform
layout (location = 4) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform bool u_IsInstanced;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out mat3 TBN;
out vec4 LightSpaceFragPos;

void main()
{
    // u_IsInstanced 플래그에 따라 적용할 modelMatrix 결정
    mat4 modelMatrix = u_IsInstanced ? instanceModel : model;

    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));

    // World-space position
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));

    // World-space normal
    vec3 N = normalize(normalMatrix * aNormal);

    // World-space tangent
    vec3 T = normalize(normalMatrix * aTangent);

    // Orthogonalize tangent against normal
    T = normalize(T - dot(T, N) * N);

    // Bitangent
    vec3 B = normalize(cross(N, T));

    Normal = N;
    TBN = mat3(T, B, N);

    TexCoords = aTexCoords;

    LightSpaceFragPos = lightSpaceMatrix * vec4(FragPos, 1.0);

    gl_Position = projection * view * modelMatrix * vec4(aPos, 1.0);
}