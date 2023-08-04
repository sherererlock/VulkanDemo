#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
precision highp float;
#define PI 3.141592653589793
#define PI2 6.283185307179586
#define EPS 1e-3
#define INV_PI 0.31830988618
#define INV_PI2 0.15915494309

#define SAMPLE_NUM 16

layout(set = 0, binding = 0) uniform sampler2D positionSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) uniform sampler2D depthSampler;
layout(set = 0, binding = 3) uniform sampler2D colorSampler;
layout(set = 0, binding = 4) uniform sampler2D roughnessSampler;
layout(set = 0, binding = 5) uniform sampler2D albedoSampler;
layout(set = 0, binding = 6) uniform sampler2D shadowSampler;

layout(set = 0, binding = 7)
uniform UniformBufferObject{
    mat4 view;
	mat4 projection;
	mat4 depthVP;
    vec4 viewPos;
} ubo;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

#include "ssrbrdf.hlsl"
#include "ssrraymarch.hlsl"
#include "ssrtexturespace.hlsl"

float getShadow(vec4 worldPos)
{
	vec4 screenPos = ubo.depthVP * worldPos;
	screenPos.xyz /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + 0.5;

	float shadowDepth = texture(shadowSampler, screenPos.xy).r;
	if(shadowDepth < screenPos.z)
		return 0.0;

	return 1.0;
}

void main() 
{
	vec3 worldPos = texture(positionSampler, inUV).xyz;
	vec3 normal = normalize(texture(normalSampler, inUV).xyz);
	float depth = texture(depthSampler, inUV).r;
	float shadow = getShadow(vec4(worldPos.xyz, 1.0));

	vec3 dirLit = texture(colorSampler, inUV).xyz;

	float isup = texture(roughnessSampler, inUV).z;
	vec3 reflectLit = vec3(0.0);

	bool intersected = false;
	if(isup > 0.8)
		reflectLit = ScreenSpaceReflectionInTS(worldPos, normal, intersected);

	vec3 color = dirLit;
	if (intersected)
		color = reflectLit;

	{
		color = pow(color, vec3(1.0 / 2.2));
	}
//	if(intersected)
//	{
//		vec4 pos = ubo.projection * ubo.view * vec4(worldPos, 1.0);
//		pos /= pos.w;
//		pos.xy = pos.xy * 0.5 + 0.5;
//		float depth1 = texture(depthSampler, pos.xy).r;
//		color = vec3(pos.z, depth, depth1);
//	}

	outColor = vec4(color, 1.0);
}