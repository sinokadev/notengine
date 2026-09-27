#version 450
layout(location=0) in vec3 aPos;
layout(location=0) out vec3 TexCoords;
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
void main(){TexCoords=aPos;vec4 p=scene.skyViewProjection*vec4(aPos,1);gl_Position=p.xyww;}
