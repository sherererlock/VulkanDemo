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

layout(location = 0) out float outAO;

void main() 
{
	vec3 viewPos = texture(positionSampler, inUV).xyz;
	vec3 viewNormal = normalize(texture(normalSampler, inUV).xyz * 2.0 - 1.0);

	ivec2 texDim = textureSize(positionSampler, 0);
	ivec2 noiseDim = textureSize(noiseSampler, 0);
	const vec2 uvScale = vec2(float(texDim.x) / float(noiseDim.x),float(texDim.y) / float(noiseDim.y));
	const vec2 noiseUV = uvScale * inUV;
	vec3 randomVec = texture(noiseSampler, noiseUV).xyz * 2.0 - 1.0;

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
		// viewPos.z 是<-zFar, -zNear>之间的值
		// viewPos.w 是<zNear, zFar>之间的值
		// sampleDepth比samplePos.z大，说明深度贴图上的点比sample point离相机越近，则这个点被遮挡，记录进AO中
		// rangeCheck 在离当前着色点半径为SSAO_RADIUS的区域内，权重越大，超出这个区域权重变小
		float sampleDepth = -texture(positionSampler, offset.xy).w;
		float rangeCheck = smoothstep(0.0, 1.0, SSAO_RADIUS / abs(viewPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}
	// 记录直接可以乘以间接光的值
	occlusion = 1.0 - (occlusion /float(SSAO_KERNEL_SIZE));

	outAO = occlusion;
}