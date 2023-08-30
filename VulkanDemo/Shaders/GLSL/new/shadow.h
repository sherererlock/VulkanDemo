#ifndef	SHADOW_H
#define SHADOW_H

#include "Common.hlsl"

float textureProj(vec3 coord, vec2 offset)
{
	float shadow = 1.0;

	if (coord.z > -1.0 && coord.z < 1.0)
	{
		float dist = texture(shadowSampler, coord.xy + offset).r;
		if (dist < coord.z)
			shadow = 0.1;
	}

	return shadow;
}


float pcf7x7(vec3 texCoord)
{
	ivec2 itexelSize = textureSize(shadowSampler, 0);

	float deltaSize = 1.0 / float(itexelSize.x);
	vec2 texelSize = vec2(deltaSize, deltaSize);

	float depth = 0.0;
	int numSamples = 0;
	int range = 3;

	for (int i = -range; i <= range; i++)
	{
		for (int j = -range; j <= range; j++)
		{
			vec2 offset = vec2(i, j) * texelSize;
			depth += textureProj(texCoord, offset);
			numSamples++;
		}
	}

	depth = depth / float(numSamples);

	return depth;
}

float PCF(vec3 coords, float filteringSize) {
	float bias = 0.0;
	int shadowIndex = int(ubo.shadowParams.x);
	if (shadowIndex == 2)
		uniformDiskSamples(coords.xy);
	else if (shadowIndex == 3)
		poissonDiskSamples(coords.xy);

	float shadow = 0.0;
	for (int i = 0; i < NUM_SAMPLES; i++)
		shadow += textureProj(coords, poissonDisk[i] * filteringSize);

	return shadow / float(NUM_SAMPLES);
}

float findBlocker(vec3 coords, float positionFromLight)
{
	int blockerNum = 0;
	float blockDepth = 0.;

	float posZFromLight = positionFromLight;

	float zReceiver = coords.z;

	float searchRadius = LIGHT_SIZE_UV * (posZFromLight - NEAR_PLANE) / posZFromLight;

	ivec2 itexelSize = textureSize(shadowSampler, 0);
	float deltaSize = 10.0 / float(itexelSize.x);

	float filteringRange = deltaSize * searchRadius;

	poissonDiskSamples(coords.xy);
	for (int i = 0; i < NUM_SAMPLES; i++) {
		float shadowDepth = texture(shadowSampler, coords.xy + poissonDisk[i] * filteringRange).r;
		if (zReceiver > shadowDepth) {
			blockerNum++;
			blockDepth += shadowDepth;
		}
	}

	if (blockerNum == 0)
		return 1.0;

	if (blockerNum == NUM_SAMPLES)
		return -1.0;

	float avgDepth = blockDepth / float(blockerNum);

	return avgDepth;
}

float PCSS(vec3 coords, vec4 positionFromLight)
{
	float blockerDepth = findBlocker(coords, positionFromLight.z);
	if (blockerDepth < 0.0)
		return 0.0;

	if (blockerDepth == 1.0)
		return 1.0;

	float wp = (coords.z - blockerDepth) * LIGHT_SIZE_UV / blockerDepth;

	ivec2 itexelSize = textureSize(shadowSampler, 0);

	float textureSize = itexelSize.x;

	float filteringSize = wp;

	return PCF(coords, filteringSize);
}

float getShadow(vec3 coord, vec4 positionFromLight)
{
	int shadowIndex = int(ubo.shadowParams.x);
	switch (shadowIndex)
	{
	case 0:
		return textureProj(coord, vec2(0.0));
	case 1:
		return pcf7x7(coord);
	case 2:
	case 3:
		ivec2 texDim = textureSize(shadowSampler, 0);
		float filteringSize = 1.0 / float(texDim.x);
		return PCF(coord, filteringSize);
	case 4:
		return PCSS(coord, positionFromLight);
	}
}

float getShadow(vec4 worldPos)
{
	vec4 positionFromLight = ubo.depthVP * worldPos;
	vec4 screenPos = positionFromLight;
	screenPos.xyz /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + 0.5;

	return getShadow(screenPos.xyz, positionFromLight);
}

#endif