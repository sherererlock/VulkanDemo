#version 450

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform samplerCube samplerEnv;

layout(push_constant) uniform PushConsts {
	layout (offset = 64) float Roughness;
	layout (offset = 68) uint numSamples;
} consts;

#define PI 3.1415926535897932384626433832795

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

vec2 hammersley2d(uint i, uint N) 
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal) 
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, normal));
	vec3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Normal Distribution function
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

void main() 
{
	vec3 pos = inPos;
	pos.y = 1.0 - pos.y;

	vec3 R = normalize(pos);
	vec3 V = R;
	vec3 N = R;

	float envMapDim = float(textureSize(samplerEnv, 0).s);
	float omegaP = 4.0 * PI / (6.0 * envMapDim * envMapDim);

	vec3 color = vec3(0.0);
	float totalWeight = 0.0;
	for(uint i = 0 ; i < consts.numSamples; i ++)
	{
		vec2 Xi = hammersley2d(i, consts.numSamples);
		vec3 H = importanceSample_GGX(Xi, consts.Roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;

		float ndotl = dot(N, L);
		if(ndotl > 0.0)
		{
			float ndoth = clamp(dot(N, H), 0.0, 1.0);
			float vdoth = clamp(dot(V, H), 0.0, 1.0);

			float pdf = D_GGX(ndoth, consts.Roughness) * ndoth / (4.0 * vdoth) + 0.0001;
			float omegaS = 1.0 / (float(consts.numSamples) * pdf);
			float mipLevel = consts.Roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0);
			color += textureLod(samplerEnv, L, mipLevel).rgb * ndotl;
			totalWeight += ndotl;
		}
	}

	color /= totalWeight;

	outColor = vec4(color, 1.0);
}