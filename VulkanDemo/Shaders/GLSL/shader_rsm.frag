#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include"macros.hlsl"
#include"FragmentInput.hlsl"

layout(set = 1, binding = 1) uniform sampler2D shadowMapSampler;
layout(location = 5) in vec4 shadowCoord;

vec2 GetRoughnessAndMetallic()
{
    vec2 roughMetalic;
	roughMetalic.x = 0.8;
	roughMetalic.y = 0.1;
	return roughMetalic;
}

#include"lighting.hlsl"
#include"shadow.hlsl"

layout(location = 0) out vec4 outColor;
void main(){
	vec3 coord = shadowCoord.xyz;
	if(shadowCoord.w > 0.0)
	{
		coord = coord / shadowCoord.w;
		coord.xy = coord.xy * 0.5 + 0.5;
	}

	float shadow = getShadow(coord);

	vec3 albedo = texture(colorSampler, fragTexCoord).rgb; 

	vec2 roughMetalic = GetRoughnessAndMetallic();
	float roughness = roughMetalic.x;
	float metallic = roughMetalic.y;

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 n = calculateNormal();
	vec3 v = normalize(ubo.viewPos.xyz - worldPos);

	vec3 color = DirectLighting(n, v, albedo, F0, roughness, metallic) * shadow;

	color = pow(color, vec3(1.0/2.2));

	outColor = vec4(color, 1.0);
}