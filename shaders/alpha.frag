#version 450
layout(location=0) out vec4 FragColor;
layout(location=1) in vec2 TexCoords;
layout(set=1,binding=0) uniform sampler2D diffuse;
layout(push_constant) uniform MaterialUniform { vec4 albedo; vec4 factors; } params;
void main(){FragColor=texture(diffuse,TexCoords)*params.albedo;if(FragColor.a<0.01)discard;}
