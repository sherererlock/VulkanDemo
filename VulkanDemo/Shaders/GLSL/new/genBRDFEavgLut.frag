#version 450

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D EmuSampler;

const float PI = 3.1415926536;

void main()
{
	ivec2 size = textureSize(EmuSampler, 0);
	vec3 color = vec3(0.0);
	float delta = 1.0 / float(size.x);
	for(int i = 0; i < size.x; i ++)
	{
		vec2 uv = vec2((float(i) + 0.5) * delta, inUV.y);
		color += texture(EmuSampler,uv).rgb;
	}

	outColor = vec4(color * delta, 1.0);
}