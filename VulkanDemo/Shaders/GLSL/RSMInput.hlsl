#ifndef	RSMINPUT
#define RSMINPUT

#define RSM_SAMPLE_COUNT 64

layout(set = 1, binding = 2) uniform sampler2D vPosSampler;
layout(set = 1, binding = 3) uniform sampler2D vNormalSampler;
layout(set = 1, binding = 4) uniform sampler2D vFluxSampler;

layout(set = 1, binding = 5) uniform Random
{
	vec4 xi[RSM_SAMPLE_COUNT];
} random;

#endif