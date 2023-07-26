#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const int SSAO_KERNEL_SIZE = 64;
layout(constant_id = 1) const float	SSAO_RADIUS = 0.5;

layout(set = 0, binding = 0) 
uniform UniformBufferObject
{
	mat4 proj;
	vec4 samples[SSAO_KERNEL_SIZE];
} ubo;

layout(set = 0, binding = 1) uniform sampler2D positionSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D noiseSampler;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outAO;

void main() 
{
	vec3 viewPos = texture(positionSampler, inUV);
	vec3 viewNormal = normalize(texture(normalSampler, inUV) * 2.0 - 1.0);

	ivec2 texDim = textureSize(positionSampler, 0);
	ivec2 noiseDim = textureSize(noiseSampler, 0);
	const vec2 uvScale = vec2(float(texDim.x) / float(noiseDim.x),float(texDim.y) / float(noiseDim.y));
	const vec2 noiseUV = uvScale * inUV;
	vec3 randomVec = texture(noiseSampler, noiseUV) * 2.0 - 1.0;

	vec3 tangent = normalize(randomVec - dot(randomVec, viewNormal) * viewNormal);
	vec3 bitangent = normalize(cross(tangent, viewNormal));
	mat3 TBN = mat3(tangent, bitangent, viewNormal);

	float occlusion = 0.0;
	const float bias = 0.025;
	for(int i = 0; i < SSAO_KERNEL_SIZE; i ++)
	{
		vec3 samplePos = TBN * ubo.samples[i].xyz;
		samplePos = viewPos + samplePos * SSAO_RADIUS;

		vec4 offset = vec4(samplePos, 1.0);
		offset = ubo.proj * offset;
		offset.xyz /= offset.w;
		offset.xy = offset.xy * 0.5 + 0.5;

		float sampleDepth = -texture(positionSampler, offset.xy).w;
		float rangeCheck = smoothstep(0.0, 1.0, SSAO_RADIUS / abs(viewPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion /float(SSAO_KERNEL_SIZE));

	outAO = occlusion;
}