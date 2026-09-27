#version 450
layout(location=0) in vec3 TexCoords;
layout(location=0) out vec4 FragColor;
layout(set=1,binding=5) uniform samplerCube skybox;
void main(){vec3 c=texture(skybox,TexCoords).rgb;FragColor=vec4(pow(c/(c+vec3(1)),vec3(1.0/2.2)),1);}
