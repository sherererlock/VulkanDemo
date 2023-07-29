#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define SSAO

#include"macros.hlsl"
#include"FragmentInput.hlsl"

layout(set = 1, binding = 1) uniform sampler2D shadowMapSampler;
layout(set = 1, binding = 2) uniform sampler2D ssaoSampler;

vec2 GetRoughnessAndMetallic()
{
    vec2 roughMetalic = texture(roughnessSampler, fragTexCoord).gb;
	roughMetalic.x = 0.8;
	roughMetalic.y = 0.1;
	return roughMetalic;
}

#include"lighting.hlsl"

layout(location = 0) out vec4 outColor;
void main(){

	vec3 color = Lighting(1.0);

	color = pow(color, vec3(1.0/2.2));

	outColor = vec4(color, 1.0);
}