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

const vec2 kOffsets2x2[5] =
{
	vec2(-1, 0), //left
	vec2(0, -1), //up
	vec2( 0,  0), // K
	vec2(1, 0), //right
	vec2(0, 1) //down
}; //k is index 3

const uint kNeighborsCount = 9;
const uint neighborCount = 5;

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

vec4 MinColors(in vec4 neighborColors[neighborCount])
{
	vec4 minColor = neighborColors[0];

	for(int iter = 1; iter < neighborCount; iter++)
	{
		minColor = min(minColor, neighborColors[iter]);
	}

	return minColor;
}

vec4 MaxColors(in vec4 neighborColors[neighborCount])
{
	vec4 maxColor = neighborColors[0];

	for(int iter = 1; iter < neighborCount; iter++)
	{
		maxColor = max(maxColor, neighborColors[iter]);
	}

	return maxColor;
}

vec4 MinColors2(in vec4 neighborColors[kNeighborsCount])
{
	vec4 minColor = neighborColors[0];

	for(int iter = 1; iter < neighborCount; iter++)
	{
		minColor = min(minColor, neighborColors[iter]);
	}

	return minColor;
}

vec4 MaxColors2(in vec4 neighborColors[kNeighborsCount])
{
	vec4 maxColor = neighborColors[0];

	for(int iter = 1; iter < neighborCount; iter++)
	{
		maxColor = max(maxColor, neighborColors[iter]);
	}

	return maxColor;
}

vec4 ConstrainHistory(vec4 neighborColors[neighborCount])
{
	vec4 previousColorMin = MinColors(neighborColors);
	vec4 previousColorMax = MaxColors(neighborColors);

	//vec4 constrainedHistory = vec4(0);
	return clamp(neighborColors[2], previousColorMin, previousColorMax);
}

// note: clips towards aabb center + p.w
vec4 clip_aabb(vec3 colorMin, vec3 colorMax, vec4 currentColor, vec4 previousColor)
{
	vec3 p_clip = 0.5 * (colorMax + colorMin);
	vec3 e_clip = 0.5 * (colorMax - colorMin);
	vec4 v_clip = previousColor - vec4(p_clip, currentColor.a);
	vec3 v_unit = v_clip.rgb / e_clip;
	vec3 a_unit = abs(v_unit);
	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));
	if (ma_unit > 1.0)
	{
		return vec4(p_clip, currentColor.a) + v_clip / ma_unit;
	}
	else
	{
		return previousColor;// point inside aabb
	}
}

vec4 Inside2Resolve(sampler2D currColorTex, sampler2D prevColorTex, vec2 velocity)
{
	vec2 deltaRes = vec2(1.0 / ubo.resolution.x, 1.0 / ubo.resolution.y);

	vec4 current3x3Colors[kNeighborsCount];
	vec4 previous3x3Colors[kNeighborsCount];

	for(uint iter = 0; iter < kNeighborsCount; iter++)
	{
		vec2 newUV = inUV + (kOffsets3x3[iter] * deltaRes);

		current3x3Colors[iter] = texture(currColorTex, newUV);

		previous3x3Colors[iter] = texture(prevColorTex, newUV + velocity);
	}

	vec4 rounded3x3Min = MinColors2(current3x3Colors);
	vec4 rounded3x3Max = MaxColors2(previous3x3Colors);

	vec4 current2x2Colors[neighborCount];
	vec4 previous2x2Colors[neighborCount];
	for(uint iter = 0; iter < neighborCount; iter++)
	{
		vec2 newUV = inUV + (kOffsets2x2[iter] * deltaRes);

		current2x2Colors[iter] = texture(currColorTex, newUV);

		previous2x2Colors[iter] = texture(prevColorTex, newUV + velocity);
	}


	vec4 min2 = MinColors(current2x2Colors);
	vec4 max2 = MaxColors(previous2x2Colors);

	//mix the 3x3 and 2x2 min maxes together
	vec4 mixedMin = mix(rounded3x3Min, min2, 0.5);
	vec4 mixedMax = mix(rounded3x3Max, max2, 0.5);

	float testVel = ubo.resolution.z - (length(velocity) * ubo.resolution.w);

	return mix(current2x2Colors[2], clip_aabb(mixedMin.rgb, mixedMax.rgb, current2x2Colors[2], ConstrainHistory(previous2x2Colors)), testVel);
}

vec4 Custom2Resolve(in float preNeighborDepths[kNeighborsCount], in float curNeighborDepths[kNeighborsCount], vec2 velocity)
{
	vec4 res = vec4(0);
	
	const float maxDepthFalloff = 0.01f;
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

void oldmain() 
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


vec3 ClampColor(vec2 uv, vec2 velocity)
{
	vec3 minColor = vec3(999.0);
	vec3 maxColor = vec3(-999.0);

	vec2 delta = vec2(1.0) / ubo.resolution.xy;

	for(int x = -1; x <= 1; x ++)
	{
		for(int y = -1; y <= 1; y ++)
		{
			vec2 newUV = uv + vec2(x, y) * delta;
			vec3 color = texture(colorSampler, newUV).rgb;
			minColor = min(color, minColor);
			maxColor = max(color, maxColor);
		}
	}

	vec3 color = texture(colorSampler, uv).rgb;
	vec3 preColor = texture(lastColorSampler, uv - velocity).rgb;

	vec3 preColorClamped = clamp(preColor, minColor, maxColor);

	return mix(preColorClamped, color, ubo.resolution.z);
}

void main()
{
	vec2 velocity = texture(velocitySampler, inUV).rg;

//	vec3 color = texture(colorSampler, inUV).rgb;
//	vec2 uv = inUV - velocity ;
//	vec3 historyColor = texture(lastColorSampler, uv).rgb;
//	outColor = vec4(mix(historyColor, color, ubo.resolution.z), 1.0);

	outColor = vec4(ClampColor(inUV, velocity), 1.0);

}