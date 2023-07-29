#version 450

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform samplerCube samplerEnv;

layout(constant_id = 0) const float exposure = 4.5;
layout(constant_id = 1) const float	gamma = 2.2;

// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

void main() 
{
	vec3 uvw = inUVW;
	uvw.y = 1.0 - uvw.y;
	vec3 color = texture(samplerEnv, uvw).rgb;

//	// Tone mapping
	color = Uncharted2Tonemap(color * exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / gamma));
	
	outColor = vec4(color, 1.0);
}