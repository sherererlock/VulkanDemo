#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define PI 3.141592653589793
#define PI2 6.283185307179586
#define EPS 1e-3
#define INV_PI 0.31830988618
#define INV_PI2 0.15915494309

layout(set = 0, binding = 0) uniform sampler2D positionSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) uniform sampler2D roughnessSampler;
layout(set = 0, binding = 3) uniform sampler2D albedoSampler;
layout(set = 0, binding = 4) uniform sampler2D emissiveSampler;
layout(set = 0, binding = 5) uniform sampler2D EmuSampler;
layout(set = 0, binding = 6) uniform sampler2D EavgSampler;
layout(set = 0, binding = 7) uniform sampler2D shadowSampler;

layout(set = 0, binding = 8)
uniform UniformBufferObject{
    mat4 view;
	mat4 projection;
	mat4 depthVP;
    vec4 viewPos;
	vec4 lights[4];
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


//https://blog.selfshadow.com/publications/s2017-shading-course/imageworks/s2017_pbs_imageworks_slides_v2.pdf
vec3 AverageFresnel(vec3 r, vec3 g)
{
	return vec3(0.087237) + 0.0230685 * g - 0.0864902 * g * g + 0.0774594 * g * g * g
		+ 0.782654 * r - 0.136432 * r * r + 0.278708 * r * r * r
		+ 0.19744 * g * r + 0.0360605 * g * g * r - 0.2586 * g * r * r;
}

vec3 MultiScatterBRDF(vec3 N, vec3 L, vec3 V, vec3 albedo, float roughness)
{
	float ndotl = clamp(dot(N, L), 0.0, 1.0);
	float ndotv = clamp(dot(N, V), 0.0, 1.0);

	vec3 Eo = texture(EmuSampler, vec2(ndotv, roughness)).rgb;
	vec3 Ei = texture(EmuSampler, vec2(ndotl, roughness)).rgb;

	vec3 Eavg = texture(EavgSampler, vec2(0.0, roughness)).rgb;

	vec3 edgetint = vec3(0.827, 0.792, 0.678);
	vec3 Favg = AverageFresnel(albedo, edgetint);

	vec3 fms = (vec3(1.0) - Ei) * (vec3(1.0) - Eo) / (PI * (vec3(1.0) - Eavg));
	vec3 fadd = Favg * Eavg / (vec3(1.0) - Favg * (vec3(1.0) - Eavg));

	return fms * fadd;
}

vec3 DirectLighting(vec3 albedo, vec3 position, vec3 N, float roughness, float metallic)
{
	vec3 Lo = vec3(0.0);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	vec3 V = normalize(ubo.viewPos.xyz - position);

	for(int i = 0; i < 4; i ++)
	{
		vec3 L = normalize(ubo.lights[i].xyz - position);
		vec3 H = normalize(V + L);

		float ndotl = max(0.0, dot(N, L));
		float ndotv = max(0.0, dot(N, V));
		float ndoth = max(0.0, dot(N, H));
		float vdoth = max(0.0, dot(V, H));

		if(ndotl > 0.0)
		{
			vec3 F = fresnelSchlick(vdoth, F0);
			float D = D_GGX_TR(ndoth, roughness);
			float G = GeometrySmith(ndotl, ndotv, roughness);

			vec3 nom = F * D * G;
			float denom = max(4.0 * ndotl * ndotv, 0.001);
			vec3 specular = nom / denom;

			vec3 fms = MultiScatterBRDF(N, L, V, albedo, roughness);

//			Lo += vec3(1.0) * (specular + fms) * ndotl;

			vec3 ks = F;
			vec3 kd = (vec3(1.0) - F);
			kd *= (1 - metallic);

			Lo += (kd * albedo / PI + specular) * ndotl;
		}
	}

	return Lo;
}

vec3 Lighting(vec3 albedo, vec3 position, vec3 normal, float roughness, float metallic, float shadow)
{
	vec3 Lo = vec3(0.0);
	
	vec3 dirLo = DirectLighting(albedo, position, normal, roughness, metallic);
	vec3 emissive = texture(emissiveSampler, inUV).rgb;
	Lo = dirLo + emissive;

	return Lo;
}

void main() 
{
	vec3 albedo = pow(texture(albedoSampler, inUV).rgb, vec3(2.2));
	vec3 position = texture(positionSampler, inUV).xyz;
	vec3 normal = normalize(texture(normalSampler, inUV).xyz);
	vec4 rm = texture(roughnessSampler, inUV);
	float roughness = rm.x;
	float metallic = rm.y;
	float shadow = getShadow(vec4(position, 1.0));

	vec3 color = Lighting(albedo, position, normal, roughness, metallic, shadow);

	color = pow(color, vec3(1.0 / 2.2));

	outColor = vec4(color, 1.0);
}