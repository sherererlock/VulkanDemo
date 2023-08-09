#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

const float PI = 3.14159265359;

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

layout(set = 0, binding = 0)
uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 viewPos;
	vec4 lightPos[4];
} ubo;

layout(push_constant) uniform PushConsts {
	layout(offset = 12) float roughness;
	layout(offset = 16) float metallic;
	layout(offset = 20) float r;
	layout(offset = 24) float g;
	layout(offset = 28) float b;
} material;

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

void main()
{
	vec3 albedo = vec3(material.r, material.g, material.b);
	vec3 N = normalize(normal);
	vec3 wo = normalize(ubo.viewPos.xyz - position.xyz);



	vec3 F0 = mix(vec3(0.04), albedo, material.metallic);

	vec3 color = vec3(0.0);
	for(int i = 0; i < 4; i ++)
	{
		vec3 wi = normalize(ubo.lightPos[i].xyz - position.xyz);
		vec3 H = normalize(wi + wo);

		float ndotv = clamp(dot(wo, N), 0.0, 1.0);
		float ndotl = clamp(dot(N, wi), 0.0, 1.0);
		float ndoth = clamp(dot(N, H), 0.0, 1.0);
		float hdotv = clamp(dot(H, wo), 0.0, 1.0);

		if(ndotl > 0.0)
		{
			vec3 F = fresnelSchlick(ndotv, F0);
			float D = D_GGX_TR(ndoth, material.roughness);
			float G = GeometrySmith(ndotv, ndotl, material.roughness);

			vec3 nom = F * D * G;
			float denom = max(4 * ndotl * ndotv, 0.0001);
			vec3 fr = nom / denom;

			color += vec3(1.0) * fr * ndotl;
		}
	}

	color += albedo * 0.02;
	color = pow(color, vec3(1.0/2.2));
	outColor = vec4(color, 1.0);
}