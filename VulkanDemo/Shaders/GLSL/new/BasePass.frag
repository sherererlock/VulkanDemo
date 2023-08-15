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
layout(location = 2) out vec4 outRoughnessMetallic;
layout(location = 3) out vec4 outAlbedo;
layout(location = 4) out vec4 outEmissive;

vec2 GetRoughnessAndMetallic()
{
	ivec2 size = textureSize(roughnessSampler, 0);
	if(size.x < 64)
		return vec2(0.8, 0.1);

	return texture(roughnessSampler, fragTexCoord).gb;
}

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;

	vec3 N = normalize(normal);
	vec3 T = normalize(tangent);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

void main() 
{
	outPos = vec4(worldPos, depth);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 N = calculateNormal();
	outNormal = vec4(N , 1.0);
	outRoughnessMetallic = vec4(GetRoughnessAndMetallic(), 0.0, 1.0);
	vec3 albedo = texture(colorSampler, fragTexCoord).rgb;
	outAlbedo = vec4(albedo, 1.0);
	outEmissive = vec4(texture(emissiveSampler, fragTexCoord).rgb * materialData.emissiveFactor, 1.0);
}