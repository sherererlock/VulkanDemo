#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) 
uniform UniformBufferObject
{
	vec4 clipPlane;
	vec4 lightPos;
	vec4 lightColor;
	mat4 depthVP;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D colorSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec3 tangent;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outFlux;

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalSampler, texcoord).xyz * 2.0 - 1.0;

	vec3 N = normalize(normal);
	vec3 T = normalize(tangent);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * ubo.clipPlane.x * ubo.clipPlane.y) / (ubo.clipPlane.y + ubo.clipPlane.x - z * (ubo.clipPlane.y - ubo.clipPlane.x));	
}

void main() 
{
	vec3 albedo = texture(colorSampler, texcoord).rgb;
	vec3 N = calculateNormal();
	outPos = vec4(worldPos, linearDepth(gl_FragCoord.z));
	outNormal = vec4(N, 1.0);

	// point light
	vec3 wi = worldPos - ubo.lightPos.xyz;
	outFlux = vec4(albedo * dot(N, wi) / (wi * wi) * ubo.lightColor.rgb, 1.0);

	// directional light
	outFlux = vec4(albedo * ubo.lightColor.rgb, 1.0);
}