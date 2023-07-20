#ifndef	FINPUT
#define FINPUT

layout(set = 0, binding = 0)
uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	mat4 depthVP;
	vec4 splitDepth;
	vec4 viewPos;
	int shadowIndex;
	float filterSize;
} ubo;

layout(set = 1, binding = 0)
uniform uboShared{
	vec4 lights[4];
	int colorCascades;
	float exposure;
	float gamma;
} uboParam;

layout(set = 2, binding = 0) uniform sampler2D colorSampler;
layout(set = 2, binding = 1) uniform sampler2D normalSampler;
layout(set = 2, binding = 2) uniform sampler2D roughnessSampler;
layout(set = 2, binding = 3) uniform sampler2D emissiveSampler;
layout(set = 2, binding = 4)
uniform MaterialData{
	vec4 baseColorFactor;
	vec3 emissiveFactor;
} materialData;

layout(push_constant) uniform PushConsts{
	layout(offset = 64) float islight;
} material;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 tangent;

#endif