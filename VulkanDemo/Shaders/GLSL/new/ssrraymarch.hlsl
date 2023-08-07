
#ifndef SSRRAYMARCH
#define SSRRAYMARCH

float GetDepth(vec4 worldPos)
{
	vec4 clipPos = ubo.projection * ubo.view * worldPos;

	return clipPos.w;
}

vec3 GetScreenUV(vec4 worldPos)
{
	vec4 screenPos = ubo.projection * ubo.view * worldPos;
	screenPos.xyz /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + 0.5;
	return screenPos.xyz;
}

bool RayMarch_w(vec3 origin, vec3 dir, out vec3 pos)
{
	float stepdis = 0.05;
	float maxDistance = 1.5;

	float currentDistance = stepdis;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		
		float depthInBuffer = texture(positionSampler, screenPos.xy).w;
		float depth = GetDepth(vec4(worldPos, 1.0));
        float thickness = depth - depthInBuffer;
		
        if (thickness > 0.001 && thickness < 1)
		{
			pos = worldPos;
			return true;
		}

		currentDistance += stepdis;
	}

	return false;
}

float GetMinDepth(vec2 uv, int level)
{
	ivec2 size = textureSize(depthSampler, level);
	ivec2 cell = ivec2(floor(size * uv));

	return texelFetch(depthSampler, cell, level).r;
}

bool RayMarch_hiz(vec3 origin, vec3 dir, out vec3 hitpos)
{
	float stepdis = 0.05;
	float maxDistance = 1.5;

	int startLevel = 3;
	int stopLevel = 0;

	int level = startLevel;
	float currentDistance = stepdis;
	while(level >= stopLevel && currentDistance <= maxDistance)
	{
		vec3 pos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(pos, 1.0));

		float depthInBuffer = GetMinDepth(screenPos.xy, level);

		if(screenPos.z - depthInBuffer > 0.00001)
		{
			if(level == 0)
			{
				hitpos = pos;
				return true;
			}

			level --;
		}
		else
		{
			level = min(10, level + 1);
			currentDistance += stepdis * level;
		}
	}

	return false;
}

vec3 ScreenSpaceReflectionGloosy(vec3 worldPos, vec3 normal)
{
	vec3 color = vec3(0.0);
	if(length(normal) < 0.001)
		return color;

	vec3 wo = normalize(ubo.viewPos.xyz - worldPos.xyz);
	float s = InitRand(gl_FragCoord.xy);
	for(int i = 0; i < SAMPLE_NUM; i ++)
	{
		float pdf;
		vec3 localDir = SampleHemisphereCos(s, pdf);
//		vec3 localDir = SampleHemisphereUniform(s, pdf);
		vec3 b1, b2;
		LocalBasis(normal, b1, b2);
		vec3 dir = normalize( mat3(b1, b2, normal) * localDir);
		vec3 pos;
		if(RayMarch_hiz(worldPos.xyz, dir, pos))
		{
			vec3 screenPos = GetScreenUV(vec4(pos, 1.0));

			vec3 indirL = texture(colorSampler, screenPos.xy).xyz;
			vec3 albedo = texture(albedoSampler, screenPos.xy).xyz;
			vec2 roughness = texture(roughnessSampler, screenPos.xy).xy;
			vec3 wo = normalize(ubo.viewPos.xyz - worldPos);
			vec3 wi = normalize(pos - worldPos);
			vec3 brdf = GetBRDF(normal, wo, wi, albedo, roughness.x, roughness.y);
		
			color += indirL * brdf * dot(wi, normal) / pdf;
		}
	}

	return color / float(SAMPLE_NUM);
}

bool RayMarch(vec3 origin, vec3 dir, out vec3 pos)
{
	float stepdis = 0.05;
	float maxDistance = 1.5;

	float currentDistance = stepdis;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		float depth = texture(depthSampler, screenPos.xy).r;
		float thickness = screenPos.z - depth;
		if(thickness > 0 && thickness < 0.01)
		{
			pos = worldPos;
			return true;
		}

		currentDistance += stepdis;
	}

	return false;
}

bool CheckUVValid(vec2 uv)
{
    return uv.x > 0.0 && uv.x < 1.0 && uv.y > 0.0 && uv.y < 1.0;
}

vec3 ScreenSpaceReflection(vec3 worldPos, vec3 normal, out bool intersected)
{
	vec3 color = vec3(0.0);
	if(length(normal) < 0.001)
		return color;

    intersected = false;
	vec3 wo = normalize(ubo.viewPos.xyz - worldPos);
	vec3 R = normalize(reflect(-wo, normal));
	vec3 pos = vec3(0.0);
	if(RayMarch(worldPos, R, pos))
	{
		vec3 screenPos = GetScreenUV(vec4(pos, 1.0));
        if (!CheckUVValid(screenPos.xy))
            return color;

        intersected = true;
        vec3 indirL = texture(colorSampler, screenPos.xy).rgb;
        vec3 albedo = texture(albedoSampler, screenPos.xy).rgb;
		//vec2 roughness = texture(roughnessSampler, screenPos.xy).xy;
		//vec3 wo = normalize(ubo.viewPos.xyz - worldPos);
		//vec3 wi = normalize(pos - worldPos);
		//vec3 brdf = GetBRDF(normal, wo, wi, albedo, roughness.x, roughness.y);

		//color = indirL * brdf * dot(wi, normal);
	
        color = indirL;
    }

	return color;
}

#endif