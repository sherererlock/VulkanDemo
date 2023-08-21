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

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

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

 float FilterCubic(in float x, in float B, in float C)
{
    float y = 0.0f;
    float x2 = x * x;
    float x3 = x * x * x;
    if(x < 1)
        y = (12 - 9 * B - 6 * C) * x3 + (-18 + 12 * B + 6 * C) * x2 + (6 - 2 * B);
    else if (x <= 2)
        y = (-B - 6 * C) * x3 + (6 * B + 30 * C) * x2 + (-12 * B - 48 * C) * x + (8 * B + 24 * C);

    return y / 6.0f;
}


// The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae

// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
vec4 SampleTextureCatmullRom(sampler2D tex, in vec2 uv, in vec2 texSize)
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    vec2 samplePos = uv * texSize;
    vec2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    vec2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    vec2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    vec2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    vec2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    vec2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    vec2 texPos0 = texPos1 - 1;
    vec2 texPos3 = texPos1 + 2;
    vec2 texPos12 = texPos1 + offset12;

    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

    vec4 result = vec4(0.0f);
    result += texture(tex, vec2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += texture(tex, vec2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += texture(tex, vec2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += texture(tex, vec2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += texture(tex, vec2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += texture(tex, vec2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += texture(tex, vec2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += texture(tex, vec2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += texture(tex, vec2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

    return result;
}

float Luminance(vec3 color)
{
	return dot(vec3(0.2127, 0.7152, 0.0722), color);
}

float rcp(float value)
{
	return 1.0/value;
}

void main()
{
	vec3 colorTotal = vec3(0.0);
	float weight = 0.0;
	vec3 neighborhoodMin = vec3(10000);
	vec3 neighborhoodMax = vec3(-10000);

	vec3 m1 = vec3(0.0);
	vec3 m2 = vec3(0.0);

	float closedDepth = 99;
	vec2 closedUV = vec2(0.0);

	vec2 deltaRes = vec2( 1.0 / ubo.resolution.x, 1.0 / ubo.resolution.y);

	for(int x = -1; x < 1; x ++)
	{
		for(int y = -1; y < 1; y ++)
		{
			vec2 uv = inUV + deltaRes * vec2(x, y);
			uv = clamp(uv, vec2(0.0), vec2(1.0));

			vec3 neighbor = texture(colorSampler, uv).rgb;
			float subSampleDistance = length(vec2(x, y));
			float subSampeWeight = FilterCubic(subSampleDistance, 1 / 3.0f, 1 / 3.0f);

			colorTotal += neighbor * subSampeWeight;
			weight += subSampeWeight;

			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);

			m1 += neighbor;
			m2 += neighbor * neighbor;

			float currentDepth = texture(depthSampler, uv).r;
			if(currentDepth < closedDepth)
			{
				closedDepth = currentDepth;
				closedUV = uv;
			}
		}
	}

	vec2 velocity = texture(velocitySampler, closedUV).rg;
	vec2 historyUV = inUV - velocity;

	vec3 currentColor = colorTotal / weight;
	vec3 historyColor = SampleTextureCatmullRom(lastColorSampler, historyUV, ubo.resolution.xy).rgb;

	float oneDividedBySampleCount = 1.0 / 9.0;
	float gamma = 1.0;
	vec3 mu = m1 * oneDividedBySampleCount;
	vec3 sigma = sqrt(abs((m2 * oneDividedBySampleCount) - (mu * mu)));
	vec3 minc = mu - gamma * sigma;
	vec3 maxc = mu + gamma * sigma;

	historyColor = clip_aabb(minc, maxc, vec4(currentColor, 1.0), vec4(clamp(historyColor, neighborhoodMin, neighborhoodMax), 1.0)).rgb;

	float sourceWeight = 0.05;
	float historyWeight = 1.0 - sourceWeight;
	vec3 compressedSource = currentColor * rcp(max(max(currentColor.r, currentColor.g), currentColor.b) + 1.0);
	vec3 compressedHistory = historyColor * rcp(max(max(historyColor.r, historyColor.g), historyColor.b) + 1.0);
	float luminanceSource = Luminance(compressedSource);
	float luminanceHistory = Luminance(compressedHistory); 
 
	sourceWeight *= 1.0 / (1.0 + luminanceSource);
	historyWeight *= 1.0 / (1.0 + luminanceHistory);
 
	vec3 result = (currentColor * sourceWeight + historyColor * historyWeight) / max(sourceWeight + historyWeight, 0.00001);

	outColor = vec4(result, 1.0);
}