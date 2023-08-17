#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D colorSampler;
layout(set = 0, binding = 1) uniform sampler2D lastColorSampler;
layout(set = 0, binding = 2) uniform sampler2D depthSampler;
layout(set = 0, binding = 3) uniform sampler2D preDepthSampler;
layout(set = 0, binding = 4) uniform sampler2D velocitySampler;
layout(set = 0, binding = 5)
uniform UniformBufferObject{
	vec4 resolution;
    vec2 jitter;
} ubo;

const vec2 kOffsets3x3[9] =
{
	vec2(-1, -1), //upper left
	vec2( 0, -1), //up
	vec2( 1, -1), //upper right
	vec2(-1,  0), //left
	vec2( 0,  0), // K
	vec2( 1,  0), //right
	vec2(-1,  1), //lower left
	vec2( 0,  1), //down
	vec2( 1,  1), //lower right
}; //k is index 4

const uint kNeighborsCount = 9;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

vec3 RGBToYCoCg(vec3 color)
{
    mat3 mat = {
        vec3(.25,  .5, .25),
        vec3(.5,   0,  -.5),
        vec3(-.25, .5, -.25)
    };
    return mat*color;
}

vec3 YCoCgToRGB(vec3 color)
{
    mat3 mat = {
        vec3(1,  1,  -1),
        vec3(1,  0,   1),
        vec3(1, -1,  -1)
    };
    return mat * color;
}

vec2 GetClosestUV(in sampler2D depths)
{
	vec2 deltaRes = vec2(1.0 / ubo.resolution.x, 1.0 / ubo.resolution.y);
	vec2 closestUV = inUV;
	float closestDepth = 1.0f;

	for(uint iter = 0; iter < kNeighborsCount; iter++)
	{
		vec2 newUV = inUV + (kOffsets3x3[iter] * deltaRes);

		float depth = texture(depths, newUV).x;

		if(depth < closestDepth)
		{
			closestDepth = depth;
			closestUV = newUV;
		}
	}

	return closestUV;
}

vec2 MinMaxDepths(in float neighborDepths[kNeighborsCount])
{
	float minDepth = neighborDepths[0];
	float maxDepth = neighborDepths[0];

	for(int iter = 1; iter < kNeighborsCount; iter++)
	{
		minDepth = min(minDepth, neighborDepths[iter]);
		minDepth = max(maxDepth, neighborDepths[iter]);
	}

	return vec2(minDepth, maxDepth);
}

vec4 Inside2Resolve(sampler2D currColorTex, sampler2D prevColorTex, vec2 velocity)
{
	vec4 color = texture(colorSampler, inUV);
	vec4 lastColor = texture(lastColorSampler, inUV + velocity);

	return mix(lastColor, color, ubo.resolution.z +  length(velocity) * 100.0);
}

vec4 Custom2Resolve(in float preNeighborDepths[kNeighborsCount], in float curNeighborDepths[kNeighborsCount], vec2 velocity)
{
	vec4 res = vec4(0);
	
	const float maxDepthFalloff = 0.1f;
	vec2 preMinMaxDepths = MinMaxDepths(preNeighborDepths);
	vec2 curMinMaxDepths = MinMaxDepths(curNeighborDepths);

	float highestDepth = min(preMinMaxDepths.x, curMinMaxDepths.x); //get the furthest
	float lowestDepth = max(preMinMaxDepths.x, curMinMaxDepths.x); //get the closest

	float depthFalloff = abs(highestDepth - lowestDepth);

	float averageDepth = 0;
	for(uint iter = 0; iter < kNeighborsCount; iter++)
	{
		averageDepth += curNeighborDepths[iter];
	}

	averageDepth /= kNeighborsCount;

	vec4 taa = Inside2Resolve(colorSampler, lastColorSampler, velocity);
	if(depthFalloff < maxDepthFalloff)
	{
		res = taa;//vec4(1, 0, 0, 1);
	}
	else
	{
		res = texture(colorSampler, inUV);
	}

	return res;
}

void main() 
{
	vec2 closestUV = GetClosestUV(depthSampler);
	vec2 offset = -texture(velocitySampler, closestUV).rg;

	vec2 deltaRes = vec2(1.0 / ubo.resolution.x, 1.0 / ubo.resolution.y);

	float currentDepths[kNeighborsCount];
	float previousDepths[kNeighborsCount];

	for(uint iter = 0; iter < kNeighborsCount; iter++)
	{
		vec2 newUV = inUV + (kOffsets3x3[iter] * deltaRes);

		currentDepths[iter] = texture(depthSampler, newUV).x;
		previousDepths[iter] = texture(preDepthSampler, newUV + offset).x;
	}

	outColor = Custom2Resolve(previousDepths, currentDepths, offset);
}