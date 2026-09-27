#version 450
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoords;
layout(location=2) in vec3 aNormal;
layout(location=3) in vec3 aTangent;
layout(location=4) in mat4 model;
layout(location=0) out vec3 FragPos;
layout(location=1) out vec2 TexCoords;
layout(location=2) out vec3 Normal;
layout(location=3) out mat3 TBN;
layout(location=6) out vec4 LightSpaceFragPos;
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

void main() {
    FragPos=vec3(model*vec4(aPos,1));
    mat3 normalMatrix=transpose(inverse(mat3(model)));
    vec3 N=normalize(normalMatrix*aNormal);
    vec3 T=mat3(model)*aTangent; T-=dot(T,N)*N;
    if(dot(T,T)<0.00001) T=cross(abs(N.y)<0.99?vec3(0,1,0):vec3(1,0,0),N);
    T=normalize(T);
    Normal=N;TBN=mat3(T,normalize(cross(N,T)),N);TexCoords=aTexCoords;
    LightSpaceFragPos=scene.lightSpaceMatrix*vec4(FragPos,1);
    gl_Position=scene.viewProjection*vec4(FragPos,1);
}
