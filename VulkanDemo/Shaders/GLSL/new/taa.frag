#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D colorSampler;
layout(set = 0, binding = 1) uniform sampler2D lastColorSampler;
layout(set = 0, binding = 2) uniform sampler2D velocitySampler;

layout(set = 0, binding = 3)
uniform UniformBufferObject{
	float alpha;
} ubo;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 color = texture(colorSampler, inUV).rgb;
	vec2 offset = -texture(velocitySampler, inUV).rg;
	vec3 lastColor = texture(lastColorSampler, inUV + offset).rgb;

	outColor = vec4(mix(lastColor, color, ubo.alpha), 1.0);
}