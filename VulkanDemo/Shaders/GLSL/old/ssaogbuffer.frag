#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) 
uniform UniformBufferObject
{
	mat4 view;
    mat4 projection;
    vec4 clipPlane;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D normalSampler;

layout(location = 0) in vec3 viewPos;
layout(location = 1) in vec3 viewNormal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec3 viewTangent;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outNormal;

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalSampler, texcoord).xyz * 2.0 - 1.0;

	vec3 N = normalize(viewNormal);
	vec3 T = normalize(viewTangent);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

//将深度值转换到相机空间下，是线性的。
float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * ubo.clipPlane.x * ubo.clipPlane.y) / (ubo.clipPlane.y + ubo.clipPlane.x - z * (ubo.clipPlane.y - ubo.clipPlane.x));	
}

void main() 
{
	vec3 N = calculateNormal();
	outPos = vec4(viewPos, linearDepth(gl_FragCoord.z));
	N = normalize(N * 0.5 + 0.5);
	outNormal = vec4(N, 1.0);
}