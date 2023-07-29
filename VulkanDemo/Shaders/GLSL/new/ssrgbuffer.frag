#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0)
uniform UniformBufferObject{
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} ubo;

layout(set = 0, binding = 1)
uniform ShadowBufferObject{
    mat4 depthVP;
    vec4 splitDepth;
    vec4 params;
}shadowUbo;

layout(set = 1, binding = 0)
uniform uboShared{
	vec4 lights[4];
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

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in float depth;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;

vec2 GetRoughnessAndMetallic()
{
	ivec2 size = textureSize(roughnessSampler, 0);
	if(size.x < 64)
		return vec2(0.8, 0.1);

    vec2 roughMetalic = texture(roughnessSampler, fragTexCoord).gb;
	roughMetalic.x = 0.8;
	roughMetalic.y = 0.1;
	return roughMetalic;
}

#include"macros.hlsl"
#include"lighting.hlsl"

// float linearDepth(float depth)
// {
// 	float z = depth * 2.0f - 1.0f; 
// 	return (2.0f * ubo.clipPlane.x * ubo.clipPlane.y) / (ubo.clipPlane.y + ubo.clipPlane.x - z * (ubo.clipPlane.y - ubo.clipPlane.x));	
// }

void main() 
{
	outPos = vec4(worldPos, depth);

	vec3 N = calculateNormal();
	outNormal = vec4(N , 1.0);
	outColor = vec4(Lighting(1.0), 1.0);
}