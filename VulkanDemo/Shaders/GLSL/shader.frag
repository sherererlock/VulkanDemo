#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include"macros.hlsl"
#include"FragmentInput.hlsl"

layout(set = 1, binding = 1) uniform sampler2D shadowMapSampler;
layout(location = 5) in vec4 shadowCoord;

vec2 GetRoughnessAndMetallic()
{
    vec2 roughMetalic = texture(roughnessSampler, fragTexCoord).gb;
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

	vec3 color = vec3(1.0);
	if(material.islight == 0)
		color = Lighting(shadow);

	color = pow(color, vec3(1.0/2.2));

	outColor = vec4(color, 1.0);
}