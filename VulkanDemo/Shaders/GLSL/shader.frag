#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include"macros.h"

#define CASCADED_COUNT 1

layout(set = 0, binding = 0) 
uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	mat4 depthVP[CASCADED_COUNT];
    vec4 viewPos;
	int shadowIndex;
	float filterSize;
	float splitDepth[CASCADED_COUNT];
} ubo;

layout(set = 1, binding = 0) 
uniform uboShared {
    vec4 lights[4];
} uboParam;

layout(set = 1, binding = 1) uniform sampler2D shadowMapSampler;

layout(set = 2, binding = 0) uniform sampler2D colorSampler;
layout(set = 2, binding = 1) uniform sampler2D normalSampler;
layout(set = 2, binding = 2) uniform sampler2D roughnessSampler;

layout(push_constant) uniform PushConsts {
	layout(offset = 64) float islight;
} material;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec4 shadowCoord;

layout(location = 0) out vec4 outColor;

#include"lighting.h"
#include"shadow.h"

void main(){
	//vec3 color = blin_phong();
	vec3 color = vec3(1.0);

	if(material.islight == 0)
		color = pbr();

	vec3 coord = shadowCoord.xyz;
	if(shadowCoord.w > 0.0)
	{
		coord = coord / shadowCoord.w;
		coord.xy = coord.xy * 0.5 + 0.5;
	}

	float shadow = getShadow(coord);
	color *= shadow;

	outColor = vec4(color, 1.0);
}