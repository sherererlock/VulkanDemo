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
layout(set = 0, binding = 2) uniform sampler2D depthSampler;
layout(set = 0, binding = 4) uniform sampler2D roughnessSampler;
layout(set = 0, binding = 5) uniform sampler2D albedoSampler;
layout(set = 0, binding = 6) uniform sampler2D emissiveSampler;
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

vec3 DirectLighting(vec3 albedo, vec3 position, vec3 N, float roughness, float metallic)
{
	vec3 Lo = vec3(0.0);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	vec3 V = normalize(ubo.viewPos.xyz - position);

	for(int i = 0; i < 1; i ++)
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
			vec3 speculer = nom / denom;

			Lo += vec3(1.0) * speculer * ndotl;
		}
	}

	return Lo;
}

vec3 Lighting(vec3 albedo, vec3 position, vec3 normal, float roughness, float metallic, float shadow)
{
	vec3 Lo = vec3(0.0);
	
	vec3 dirLo = DirectLighting(albedo, position, normal, roughness, metallic) * shadow;
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
	float shadow = getShadow(vec4(position.xyz, 1.0));

	vec3 color = Lighting(albedo, position, normal, roughness, metallic, shadow);

	color = pow(color, vec3(1.0 / 2.2));

	outColor = vec4(color, 1.0);
}