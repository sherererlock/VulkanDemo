#ifndef	IBLINPUT
#define IBLINPUT

layout(set = 1, binding = 2) uniform samplerCube irradianceCubeMapSampler;
layout(set = 1, binding = 3) uniform samplerCube prefilterCubeMapSampler;
layout(set = 1, binding = 4) uniform sampler2D BRDFLutSampler;

#endif