#ifndef	IBLINPUT
#define IBLINPUT

layout(set = 1, binding = 4) uniform samplerCube irradianceCubeMapSampler;
layout(set = 1, binding = 5) uniform samplerCube prefilterCubeMapSampler;
layout(set = 1, binding = 6) uniform sampler2D BRDFLutSampler;

#endif

