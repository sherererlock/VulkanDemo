#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) 
uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	mat4 depthVP;
    vec4 viewPos;
} ubo;

layout(set = 1, binding = 0) 
uniform uboShared {
    vec4 lights[4];
} uboParam;

layout(set = 1, binding = 1) uniform sampler2D shadowMapSampler;

layout(set = 2, binding = 0) uniform sampler2D colorSampler;
layout(set = 2, binding = 1) uniform sampler2D normalSampler;
layout(set = 2, binding = 2) uniform sampler2D roughnessSampler;


#define PI 3.1415192654

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec4 shadowCoord;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConsts {
	layout(offset = 64) float islight;
} material;


float D_GGX_TR(vec3 n, vec3 h, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float ndoth = max(dot(n, h), 0.0);
	float ndoth2 = ndoth * ndoth;

	float nom = a2;
	float denom = ndoth2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return nom / max(denom, 0.0001f);
}

float GeometrySchlickGGX(float dotp, float k)
{
	float nom = dotp;
	float denom = dotp * (1.0 - k) + k;
	return nom / denom;
}

float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0f;

	float ndotv = max(dot(n, v), 0.0);
	float ndotl = max(dot(n, l), 0.0);

	return GeometrySchlickGGX(ndotv, k) * GeometrySchlickGGX(ndotl, k);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;

	vec3 N = normalize(normal);
	vec3 T = normalize(tangent);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

vec3 blin_phong()
{
	vec4 color = texture(colorSampler, fragTexCoord) ;

	//vec3 N = normalize(normal);

	vec3 N = calculateNormal();

	vec3 L = normalize(uboParam.lights[0].xyz - worldPos);
	vec3 V = normalize(ubo.viewPos.xyz - worldPos);
	
	vec3 R = reflect(L, N);
	vec3 diffuse = max(dot(N, L), 0.15) * color.rgb;
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
	return diffuse + specular;
}

vec3 pbr()
{
	vec3 albedo = pow(texture(colorSampler, fragTexCoord).rgb, vec3(2.2)); // error
	 
	vec2 roughMetalic = texture(roughnessSampler, fragTexCoord).gb;
	float roughness = roughMetalic.x;
	float metallic = roughMetalic.y;

	float shadow = 1.0;
	float dist = texture(shadowMapSampler, shadowCoord.xy).r;
	if ( shadowCoord.w > 0.0 && dist  > shadowCoord.z ) 
	{
		shadow = 0.0;
	}

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo.xyz, metallic);

	vec3 texnormal = calculateNormal();

	vec3 n = texnormal;
	vec3 v = normalize(ubo.viewPos.xyz - worldPos);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < 1; i ++)
	{
		vec3 l = normalize(uboParam.lights[i].xyz - worldPos);
		float ndotl = dot(n, l);
		if(ndotl > 0.0)
		{
			ndotl = clamp(dot(n, l), 0.0, 1.0);

			vec3 h = normalize(v + l);
			//float distance = length(uboParam.lights[i].xyz - worldPos);
			//float atten = 1.0 / (distance * distance);
			//vec3 irradiance = vec3(1.0) * atten;

			float ndf = D_GGX_TR(n, h, roughness);
			float g = GeometrySmith(n, v, l, roughness);
			vec3 f = fresnelSchlick(max(dot(v, n), 0.0), F0);

			vec3 ks = f;
			vec3 kd = (vec3(1.0)-f);
			kd *= (1 - metallic);
	
			vec3 nom = ndf * g * f;
			float denom = 4 * clamp(dot(n, v), 0.0, 1.0) * ndotl + 0.001;
			vec3 specular = nom / denom;
			Lo += (kd * albedo / PI + specular)* ndotl;
		}
	}

	vec3 ambient = vec3(0.03) * albedo;
	vec3 color = ambient + Lo;

	color *= shadow;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  

	return color;
}

void main(){
	//vec3 color = blin_phong();
	vec3 color = vec3(1.0);

	if(material.islight == 0)
		color = pbr();

	outColor = vec4(color, 1.0);
}