#version 450

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;
layout (constant_id = 0) const uint NUM_SAMPLES = 1024;

const float PI = 3.1415926536;

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

// Geometric Shadowing function
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float k = (roughness * roughness) / 2.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

vec2 BRDF(float ndotv, float roughness)
{
	vec3 V;
	V.x = sqrt(1.0 - ndotv * ndotv);
	V.y = 0.0;
	V.z = ndotv;
	vec3 N = vec3(0.0, 0.0, 1.0);

	vec2 lut = vec2(0.0);
	for(int i = 0; i < NUM_SAMPLES; i ++)
	{
		vec2 Xi = hammersley2d(i, NUM_SAMPLES);
		vec3 H = importanceSample_GGX(Xi, roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;

		float ndotv = max(dot(N, V), 0.0);
		float ndotl = max(dot(N, L), 0.0);
		float ndoth = max(dot(N, H), 0.0);
		float vdoth = max(dot(H, V), 0.0);

		if(ndotl > 0.0)
		{
			float G = G_SchlicksmithGGX(ndotl, ndotv, roughness);
			float G_Vis = (G * vdoth) / (ndoth * ndotv);
			float Fc = pow(1.0 - vdoth, 5.0);
			lut += vec2((1.0 - Fc) * G_Vis, Fc * G_Vis);
		}
	}

	return lut;
}

void main() 
{
	outColor = vec4(BRDF(inUV.s, inUV.t), 0.0, 1.0);
}