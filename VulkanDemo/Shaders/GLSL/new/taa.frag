#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define PI 3.141592653589793
#define PI2 6.283185307179586
#define EPS 1e-3
#define INV_PI 0.31830988618
#define INV_PI2 0.15915494309

#define SAMPLE_NUM 16

layout(set = 0, binding = 0) uniform sampler2D colorSampler;
layout(set = 0, binding = 1) uniform sampler2D lastColorSampler;

layout(set = 0, binding = 2)
uniform UniformBufferObject{
	mat4 view;
	mat4 projection;
	mat4 lastView;
	mat4 lastProj;
	float alpha;
} ubo;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 color = texture(colorSampler, inUV).rgb;
	vec3 lastColor = texture(lastColorSampler, inUV).rgb;

	outColor = vec4(mix(lastColor, color, ubo.alpha), 1.0);

}