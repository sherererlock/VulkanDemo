#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.141592653589793
#define PI2 6.283185307179586
#define EPS 1e-3
#define INV_PI 0.31830988618
#define INV_PI2 0.15915494309

#define SAMPLE_NUM 16

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

float Rand1(inout float p) {
  p = fract(p * .1031);
  p *= p + 33.33;
  p *= p + p;
  return fract(p);
}

vec2 Rand2(inout float p) {
  return vec2(Rand1(p), Rand1(p));
}

float InitRand(vec2 uv) {
  vec3 p3  = fract(vec3(uv.xyx) * .1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

vec3 SampleHemisphereUniform(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = uv.x;
  float phi = uv.y * PI2;
  float sinTheta = sqrt(1.0 - z*z);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = INV_PI2;
  return dir;
}

vec3 SampleHemisphereCos(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = sqrt(1.0 - uv.x);
  float phi = uv.y * PI2;
  float sinTheta = sqrt(uv.x);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = z * INV_PI;
  return dir;
}

void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
  float sign_ = sign(n.z);
  if (n.z == 0.0) {
    sign_ = 1.0;
  }
  float a = -1.0 / (sign_ + n.z);
  float b = n.x * n.y * a;
  b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
  b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

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
	float maxDistance = 1.5;

	float currentDistance = stepdis;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		float depth = texture(depthSampler, screenPos.xy).r;
		if(screenPos.z - depth > 0.00001)
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
	float maxDistance = 1.5;

	float currentDistance = stepdis;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		float depthInBuffer = texture(positionSampler, screenPos.xy).w;
		float depth = GetDepth(vec4(worldPos, 1.0));
		if(depth - depthInBuffer > 0.00001)
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

vec3 ScreenSpaceReflection(vec3 worldPos, vec3 normal)
{
	vec3 color = vec3(0.0);
	if(length(normal) < 0.001)
		return color;

	vec3 wo = normalize(ubo.viewPos.xyz - worldPos.xyz);
	vec3 R = normalize(reflect(-wo, normal));
	vec3 pos;
	if(RayMarch_w(worldPos.xyz, R, pos))
	{
		vec3 screenPos = GetScreenUV(vec4(pos, 1.0));

		vec3 indirL = texture(colorSampler, screenPos.xy).xyz;
		vec3 albedo = texture(albedoSampler, screenPos.xy).xyz;
		vec2 roughness = texture(roughnessSampler, screenPos.xy).xy;
		vec3 wo = normalize(ubo.viewPos.xyz - worldPos);
		vec3 wi = normalize(pos - worldPos);
		vec3 brdf = GetBRDF(normal, wo, wi, albedo, roughness.x, roughness.y);
		
		color = indirL * brdf * dot(wi, normal);
		color = indirL;
	}

	return color;
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
//		vec3 localDir = SampleHemisphereCos(s, pdf);
		vec3 localDir = SampleHemisphereUniform(s, pdf);
		vec3 b1, b2;
		LocalBasis(normal, b1, b2);
		vec3 dir = normalize( mat3(b1, b2, normal) * localDir);
		vec3 pos;
		if(RayMarch_w(worldPos.xyz, dir, pos))
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

void main() 
{
	vec3 worldPos = texture(positionSampler, inUV).xyz;
	vec3 normal = normalize(texture(normalSampler, inUV).xyz);
	float depth = texture(depthSampler, inUV).r;
	float shadow = getShadow(vec4(worldPos.xyz, 1.0));

	vec3 dirLit = texture(colorSampler, inUV).xyz;

	vec3 reflectLit = ScreenSpaceReflectionGloosy(worldPos, normal);

	vec3 color = dirLit + reflectLit;

	color = pow(color, vec3(1.0 / 2.2));
	outColor = vec4(color, 1.0);
}