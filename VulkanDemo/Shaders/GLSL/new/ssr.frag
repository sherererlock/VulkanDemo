#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.141592653589793
#define PI2 6.283185307179586
#define EPS 1e-3

layout(set = 0, binding = 0) uniform sampler2D positionSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) uniform sampler2D depthSampler;
layout(set = 0, binding = 3) uniform sampler2D colorSampler;
layout(set = 0, binding = 4) uniform sampler2D roughnessSampler;
layout(set = 0, binding = 5) uniform sampler2D albedoSampler;
layout(set = 0, binding = 6) uniform sampler2D shadowSampler;

layout(set = 0, binding = 7)
uniform UniformBufferObject{
    mat4 vp;
	mat4 depthVP;
    vec4 viewPos;
} ubo;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

float D_GGX_TR(float ndoth, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float nom = a2;
	float denom = ndoth * ndoth * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return nom / max(denom, 0.0001f);
}

float GeometrySchlickGGX(float dotp, float k)
{
	float nom = dotp;
	float denom = dotp * (1.0 - k) + k;
	return nom / denom;
}

float GeometrySmith(float ndotv, float ndotl, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0f;

	return GeometrySchlickGGX(ndotv, k) * GeometrySchlickGGX(ndotl, k);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

float getShadow(vec4 worldPos)
{
	vec4 screenPos = ubo.depthVP * worldPos;
	screenPos.xyz /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + 0.5;

	float shadowDepth = texture(shadowSampler, screenPos.xy).r;
	if(shadowDepth < screenPos.z)
		return 0.0;

	return 1.0;
}

vec3 GetScreenUV(vec4 worldPos)
{
	vec4 screenPos = ubo.vp * worldPos;
	screenPos.xyz /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + 0.5;
	return screenPos.xyz;
}

float GetDepth(vec4 worldPos)
{
	vec4 clipPos = ubo.vp * worldPos;

	return clipPos.w;
}

vec2 GetRoughness(vec2 uv)
{
	 return texture(roughnessSampler, uv).rg;
}

bool RayMarch(vec3 origin, vec3 dir, out vec3 pos)
{
	float stepdis = 0.05;
	float maxDistance = 1;

	float currentDistance = stepdis;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		float depth = texture(depthSampler, screenPos.xy).r;
		if(screenPos.z - depth > 0.01)
		{
			pos = worldPos;
			return true;
		}

		currentDistance += stepdis;
	}

	return false;
}

bool RayMarch_w(vec3 origin, vec3 dir, out vec3 pos)
{
	float stepdis = 0.05;
	float maxDistance = 3;

	float currentDistance = stepdis;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		float depthInBuffer = texture(positionSampler, screenPos.xy).w;
		float depth = GetDepth(vec4(worldPos, 1.0));
		if(depth - depthInBuffer > 0.01)
		{
			pos = worldPos;
			return true;
		}

		currentDistance += stepdis;
	}

	return false;
}

vec3 GetBRDF(vec3 N, vec3 wo, vec3 wi, vec3 albedo, float roughness, float metallic)
{
	vec3 brdf;
	vec3 H = normalize(wo + wi);

	float NdotV = clamp(dot(N, wo), 0.0, 1.0);
	float NdotL = clamp(dot(N, wi), 0.0, 1.0);
	float NdotH = clamp(dot(N, H), 0.0, 1.0);
	float HdotV = clamp(dot(H, wo), 0.0, 1.0);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F = fresnelSchlick(HdotV, F0);

	float D = D_GGX_TR(NdotH, roughness);
	float G = GeometrySmith(NdotV, NdotL, roughness);

	vec3 nom = F * D * G;
	float dnom = 4 * NdotL * NdotV + 0.0001;
	vec3 fs = nom / dnom;

	vec3 fd = albedo / PI;
	vec3 kd = (vec3(1.0) - fs)*( 1.0 - metallic);

	return kd*fd + fs;
}

vec3 ScreenSpaceReflection(vec3 worldPos, vec3 normal, float depth)
{
	vec3 color = vec3(0.0);
	if(length(normal) < 0.001)
		return color;

	vec3 wo = normalize(ubo.viewPos.xyz - worldPos.xyz);
	vec3 R = normalize(reflect(-wo, normal));
	vec3 pos;
	if(RayMarch(worldPos.xyz, R, pos))
	{
		vec3 screenPos = GetScreenUV(vec4(pos, 1.0));

		vec3 indirL = texture(colorSampler, screenPos.xy).xyz;
		vec3 albedo = texture(albedoSampler, screenPos.xy).xyz;
		vec2 roughness = texture(roughnessSampler, screenPos.xy).xy;
		vec3 wo = normalize(ubo.viewPos.xyz - worldPos);
		vec3 wi = normalize(pos - worldPos);
		vec3 brdf = GetBRDF(normal, wo, wi, albedo, roughness.x, roughness.y);
		
		color = indirL * brdf * dot(wi, normal);
	}

	return color;
}

void main() 
{

	vec3 worldPos = texture(positionSampler, inUV).xyz;
	vec3 normal = normalize(texture(normalSampler, inUV).xyz);
	float depth = texture(depthSampler, inUV).r;
	float shadow = getShadow(vec4(worldPos.xyz, 1.0));

	vec3 dirLit = texture(colorSampler, inUV).xyz;

	vec3 reflectLit = ScreenSpaceReflection(worldPos, normal, depth);

	vec3 color = dirLit * shadow + reflectLit;

	color = pow(color, vec3(1.0 / 2.2));
	outColor = vec4(color, 1.0);
}