
#ifndef SSRTEXTURESPACE
#define SSRTEXTURESPACE

float linearDepth(float depth)
{
	const float nearPlane = 0.1f;
	const float farPlane = 250.0f;

	float z = depth * 2.0f - 1.0f;
	return (2.0f * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}


void ComputePositonAndReflection_vs(vec3 origin, vec3 R, out vec3 originTS, out vec3 rTS, out float maxTraceDistance)
{
	vec3 end = origin + R * 1000.0;
	end /= (end.z < 0 ? end.z : 1.0);

	vec4 originCS = ubo.projection * vec4(origin, 1.0);
	vec4 endCS = ubo.projection * vec4(end, 1.0);

	originCS /= originCS.w;
	endCS /= endCS.w;

	vec3 rCS = normalize((endCS - originCS).xyz);

	originCS.xy = originCS.xy * vec2(0.5, 0.5) + vec2(0.5, 0.5);
	rCS.xy = rCS.xy * vec2(0.5, 0.5);

	originTS = originCS.xyz;
	rTS = rCS.xyz;

	maxTraceDistance = rTS.x >= 0 ? (1.0 - originTS.x) / rTS.x : -originTS.x / rTS.x;
	maxTraceDistance = min(maxTraceDistance, rTS.y < 0.0 ? -originTS.y / rTS.y : (1.0 - originTS.y) / rTS.y);
	maxTraceDistance = min(maxTraceDistance, rTS.z < 0.0 ? -originTS.z / rTS.z : (1.0 - originTS.z) / rTS.z);
}


void ComputePositonAndReflection(vec3 origin, vec3 R, out vec3 originTS, out vec3 rTS, out float maxTraceDistance)
{
	vec3 end = origin + R * 1000.0;
	mat4 vp = ubo.projection * ubo.view;

	vec4 originCS = vp * vec4(origin, 1.0);

	vec4 endVS = ubo.view * vec4(end, 1.0);
	endVS /= (endVS.z < 0 ? endVS.z : 1.0);
	vec4 endCS = ubo.projection * endVS;

	originCS /= originCS.w;
	endCS /= endCS.w;

	vec3 rCS = normalize((endCS - originCS).xyz);

	originCS.xy = originCS.xy * vec2(0.5, 0.5) + vec2(0.5, 0.5);
	rCS.xy = rCS.xy * vec2(0.5, 0.5);

	originTS = originCS.xyz;
	rTS = rCS.xyz;
	
	maxTraceDistance =  rTS.x >= 0 ? (1.0 - originTS.x) / rTS.x : -originTS.x / rTS.x;
	maxTraceDistance = min(maxTraceDistance, rTS.y < 0.0 ? -originTS.y / rTS.y : (1.0 - originTS.y) / rTS.y);
	maxTraceDistance = min(maxTraceDistance, rTS.z < 0.0 ? -originTS.z / rTS.z : (1.0 - originTS.z) / rTS.z); 
}

bool FindIntersectionLinear(vec3 origin, vec3 R, float maxTraceDistance, out vec3 intersection)
{
	const int MAX_ITERATION = 2000;
	const float MAX_THICKNESS = 0.1;
	const float MIN_THICKNESS = 0.01;
	// all in texture space[0, 1]^3
	vec3 end = origin + R * maxTraceDistance;

	vec3 dp = end - origin;
	ivec2 size = textureSize(depthSampler, 0);
	ivec2 originPos = ivec2(origin.xy * size);
	ivec2 endPos = ivec2(end.xy * size);
	ivec2 dp2 = endPos - originPos;
	int maxdist = max(abs(dp2.x), abs(dp2.y));
	vec3 dp3 = dp;
	dp /= float(maxdist);

	vec3 rayPos = origin + dp;
	vec3 rayDir = dp.xyz;
	vec3 rayStartPos = rayPos;

	int hitIndex = -1;
	for(int i = 0; i <= maxdist && i < MAX_ITERATION; i ++)
	{
		vec3 rayPos0 = rayPos + rayDir * 0;
		//vec3 rayPos1 = rayPos + rayDir * 1;
		//vec3 rayPos2 = rayPos + rayDir * 2;
		//vec3 rayPos3 = rayPos + rayDir * 3;

		//float depth3 = texture(depthSampler, rayPos3.xy).r;
		//float depth2 = texture(depthSampler, rayPos2.xy).r;
		//float depth1 = texture(depthSampler, rayPos1.xy).r;
		//float depth0 = texture(depthSampler, rayPos0.xy).r;

		//float depth3 = texture(positionSampler, rayPos3.xy).w;
		//float depth2 = texture(positionSampler, rayPos2.xy).w;
		//float depth1 = texture(positionSampler, rayPos1.xy).w;
		float depth0 = texture(positionSampler, rayPos0.xy).w;

		//{
		//	float thickness = rayPos3.z - depth3;
		//	hitIndex = (thickness >= MIN_THICKNESS      && thickness < MAX_THICKNESS) ? (i + 3) : hitIndex;
		//}

		//{
		//	float thickness = rayPos2.z - depth2;
		//	hitIndex = (thickness >= MIN_THICKNESS && thickness < MAX_THICKNESS) ? (i + 2) : hitIndex;
		//}

		//{
		//	float thickness = rayPos1.z - depth1;
		//	hitIndex = (thickness >= MIN_THICKNESS && thickness < MAX_THICKNESS) ? (i + 1) : hitIndex;
		//}

		//{
		//	float thickness = rayPos0.z - depth0;
		//	hitIndex = (thickness >= MIN_THICKNESS && thickness < MAX_THICKNESS) ? (i + 0) : hitIndex;
		//}

		{
			float depth = linearDepth(rayPos0.z);
			float thickness = depth - depth0;
			hitIndex = (thickness >= MIN_THICKNESS && thickness < MAX_THICKNESS) ? (i + 0) : hitIndex;
		}

		if(hitIndex != -1) break;
		
		rayPos = rayPos0 + rayDir;
	}

	bool intersected = hitIndex >= 0;
	intersection = rayStartPos.xyz + rayDir.xyz * hitIndex;
	//intersection.z = float(hitIndex) / 100.0f;
	
	return intersected;
}

vec3 ScreenSpaceReflectionInTS(vec3 worldPos, vec3 normal, out bool intersected)
{
	vec3 color = vec3(0.0);
	if(length(normal) < 0.001)
		return color;

	vec3 wo = normalize(ubo.viewPos.xyz - worldPos.xyz);
	vec3 R = normalize(reflect(-wo, normal));

	vec3 originTS;
	vec3 rTs;
	float maxTraceDistance;
	ComputePositonAndReflection(worldPos, R, originTS, rTs, maxTraceDistance);
	vec3 intersection = vec3(0.0);
	intersected = FindIntersectionLinear(originTS, rTs, maxTraceDistance, intersection);
	if(intersected)
	{
		vec3 indirL = texture(colorSampler, intersection.xy).xyz;
        vec3 pos = texture(positionSampler, intersection.xy).xyz;
		//vec3 albedo = texture(albedoSampler, intersection.xy).xyz;
		//vec2 roughness = texture(roughnessSampler, intersection.xy).xy;
		//vec3 wo = normalize(ubo.viewPos.xyz - worldPos);
		//vec3 wi = normalize(pos - worldPos);
		//vec3 brdf = GetBRDF(normal, wo, wi, albedo, roughness.x, roughness.y);
		//
		//color = indirL * brdf * dot(wi, normal);
        color = indirL;
    }

	return color;
}

vec3 ScreenSpaceReflectionInTS1(vec3 worldPos, vec3 normal, out bool intersected)
{
	vec3 color = vec3(0.0);
	if (length(normal) < 0.001)
		return color;

	vec3 samplePos = (ubo.view * vec4(worldPos, 1.0)).xyz;
	vec3 normalVS = (ubo.view * vec4(normal, 0.0)).xyz;
	normalVS = normalize(normalVS);

	vec3 viewPos = (ubo.view * ubo.viewPos).xyz;
	vec3 cameraToSample = normalize(samplePos - viewPos);

	vec3 R = normalize(reflect(cameraToSample, normal));

	vec3 originTS;
	vec3 rTs;
	float maxTraceDistance;
	ComputePositonAndReflection_vs(samplePos, R, originTS, rTs, maxTraceDistance);

	vec3 intersection = vec3(0.0);
	intersected = FindIntersectionLinear(originTS, rTs, maxTraceDistance, intersection);
	if (intersected)
	{
		color = intersection;
	}

	return color;
}


#endif