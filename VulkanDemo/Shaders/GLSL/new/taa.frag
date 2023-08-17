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

void main() 
{
	vec3 color = texture(colorSampler, inUV).rgb;

	vec2 closestUV = GetClosestUV(depthSampler);
	vec2 offset = texture(velocitySampler, closestUV).rg;

	vec2 preUV = inUV - offset;
	vec3 lastColor;

    if(min(preUV.x, preUV.y) < 0 || max(preUV.x, preUV.y) > 1)
        lastColor = color;
    else
        lastColor = texture(lastColorSampler, preUV).rgb;

    vec3 minNeighborhood = RGBToYCoCg(color);
    vec3 maxNeighborhood = minNeighborhood;
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            vec3 neighborhoodColor = texture(colorSampler, inUV + vec2(x, y) * ubo.resolution.xy).rgb;
            neighborhoodColor = RGBToYCoCg(neighborhoodColor);
            minNeighborhood = min(minNeighborhood, neighborhoodColor);
            maxNeighborhood = max(maxNeighborhood, neighborhoodColor);
        }
    }

    lastColor = RGBToYCoCg(lastColor);
    lastColor = clamp(lastColor, minNeighborhood, maxNeighborhood);
    lastColor = YCoCgToRGB(lastColor);

	outColor = vec4(mix(lastColor, color, ubo.resolution.z +  length(offset) * 100.0), 1.0);
}