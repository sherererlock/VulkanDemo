#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D positionSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) uniform sampler2D depthSampler;
layout(set = 0, binding = 3) uniform sampler2D colorSampler;
layout(set = 0, binding = 4) uniform sampler2D shadowSampler;

layout(set = 0, binding = 5)
uniform UniformBufferObject{
    mat4 view;
    mat4 proj;
	mat4 depthVP;
    vec4 viewPos;
} ubo;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

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
	vec4 screenPos = ubo.proj * ubo.view * worldPos;
	screenPos.xyz /= screenPos.w;
	screenPos.xyz = screenPos.xyz * 0.5 + 0.5;
	return screenPos.xyz;
}

float GetDepth(vec4 worldPos)
{
	vec4 screenPos = ubo.proj * ubo.view * worldPos;

	return screenPos.w;
}

bool RayMarch(vec3 origin, vec3 dir, out vec3 pos)
{
	float step = 0.05;
	float maxDistance = 10;

	float currentDistance = step;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		float depth = texture(depthSampler, screenPos.xy).r;
		if(screenPos.z - depth > 0.0001)
			pos = worldPos;
			return true;
	}

	return false;
}

bool RayMarch_w(vec3 origin, vec3 dir, out vec3 pos)
{
	float step = 0.05;
	float maxDistance = 10;

	float currentDistance = step;
	while (currentDistance < maxDistance)
	{
		vec3 worldPos = origin + dir * currentDistance;
		vec3 screenPos = GetScreenUV(vec4(worldPos, 1.0));
		float depthInBuffer = texture(positionSampler, screenPos.xy).w;
		float depth = GetDepth(vec4(worldPos, 1.0));
		if(depth - depthInBuffer > 0.0001)
		{
			pos = worldPos;
			return true;
		}
	}

	return false;
}

void main() 
{
	vec3 color = vec3(0.0);

	vec3 worldPos = texture(positionSampler, inUV).xyz;
	vec3 normal = normalize(texture(normalSampler, inUV).xyz * 2.0 - 1.0);
	float depth = texture(depthSampler, inUV).r;
	float shadow = getShadow(vec4(worldPos.xyz, 1.0));

	vec3 wo = normalize(ubo.viewPos.xyz - worldPos.xyz);
	vec3 R = normalize(reflect(-wo, normal));

	vec3 pos;
	if(RayMarch_w(worldPos.xyz, R, pos))
	{
		vec3 screenPos = GetScreenUV(vec4(pos, 1.0));
		color = texture(colorSampler, screenPos.xy).xyz;
	}
	else
	{
		color = texture(colorSampler, inUV).xyz;
	}

	color = texture(colorSampler, inUV).xyz;
	color *= shadow;

	color = pow(color, vec3(1.0 / 2.2));
	outColor = vec4(color, 1.0);

}