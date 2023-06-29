#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUM_SAMPLES 10
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10
#define EPS 1e-3

#define PI 3.141592653589793
#define PI2 6.283185307179586

#define CASCADED_COUNT 4

layout(set = 0, binding = 0) 
uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	mat4 depthVP[CASCADED_COUNT];
    vec4 viewPos;
	int shadowIndex;
	float filterSize;
	int splitDepth[CASCADED_COUNT];
} ubo;

layout(set = 1, binding = 0) 
uniform uboShared {
    vec4 lights[4];
} uboParam;

//layout(set = 1, binding = 1) uniform sampler2D shadowMapSampler;
layout(set = 1, binding = 1) uniform sampler2DArray shadowMapSampler;

layout(set = 2, binding = 0) uniform sampler2D colorSampler;
layout(set = 2, binding = 1) uniform sampler2D normalSampler;
layout(set = 2, binding = 2) uniform sampler2D roughnessSampler;

layout(push_constant) uniform PushConsts {
	layout(offset = 64) float islight;
} material;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 worldPos;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec4 viewPos;

layout(location = 0) out vec4 outColor;

/*----------------Lighting----------------*/

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

	vec3 L = normalize(uboParam.lights[0].xyz - worldPos.xyz);
	vec3 V = normalize(ubo.viewPos.xyz - worldPos.xyz);
	
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

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo.xyz, metallic);

	vec3 texnormal = calculateNormal();

	vec3 n = texnormal;
	vec3 v = normalize(ubo.viewPos.xyz - worldPos.xyz);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < 1; i ++)
	{
		vec3 l = normalize(uboParam.lights[i].xyz - worldPos.xyz);
		float ndotl = dot(n, l);
		if(ndotl > 0.0)
		{
			ndotl = clamp(dot(n, l), 0.0, 1.0);

			vec3 h = normalize(v + l);
			//float distance = length(uboParam.lights[i].xyz - worldPos.xyz);
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

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  

	return color;
}

/*----------------Lighting----------------*/

/*-----------------shadow-----------------*/

#define FRUSTUM_SIZE 400
#define NEAR_PLANE 0.01
#define LIGHT_WORLD_SIZE 5.
#define LIGHT_SIZE_UV LIGHT_WORLD_SIZE  / FRUSTUM_SIZE

float rand_1to1(float x ) { 
  // -1 -1
  return fract(sin(x)*10000.0);
}

float rand_2to1(vec2 uv ) { 
  // 0 - 1
	const float a = 12.9898, b = 78.233, c = 43758.5453;
	float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

vec2 poissonDisk[NUM_SAMPLES];
void poissonDiskSamples( const in vec2 randomSeed ) {

  float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
  float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

  float angle = rand_2to1( randomSeed ) * PI2;
  float radius = INV_NUM_SAMPLES;
  float radiusStep = radius;

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
    radius += radiusStep;
    angle += ANGLE_STEP;
  }
}

void uniformDiskSamples( const in vec2 randomSeed ) {

  float randNum = rand_2to1(randomSeed);
  float sampleX = rand_1to1( randNum ) ;
  float sampleY = rand_1to1( sampleX ) ;

  float angle = sampleX * PI2;
  float radius = sqrt(sampleY);

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonDisk[i] = vec2( radius * cos(angle) , radius * sin(angle)  );

    sampleX = rand_1to1( sampleY ) ;
    sampleY = rand_1to1( sampleX ) ;

    angle = sampleX * PI2;
    radius = sqrt(sampleY);
  }
}

float Bias(float depthCtr)
{
	ivec2 texDim = textureSize(shadowMapSampler, 0).xy;
	vec3 lightDir = normalize(uboParam.lights[0].xyz - worldPos.xyz);
	vec3 normal = normalize(normal);
	float m = FRUSTUM_SIZE / float(texDim.x) / 2.0; //正交矩阵宽高/shadowMapSize/2
	float bias = max(m, m * (1.0 - dot(normal, lightDir))) * depthCtr;

	return bias;
}

float getShadowBias(float c, float filterRadiusUV){
	ivec2 texDim = textureSize(shadowMapSampler, 0).xy;
	vec3 normal = normalize(normal);
	vec3 lightDir = normalize(uboParam.lights[0].xyz - worldPos.xyz);
	float fragSize = (1. + ceil(filterRadiusUV)) * (FRUSTUM_SIZE / texDim.x / 2.);
	return max(fragSize, fragSize * (1.0 - dot(normal, lightDir))) * c;
}

float textureProj(vec3 coord, vec2 offset, uint index)
{
	float shadow = 1.0;

	if(coord.z > -1.0 && coord.z < 1.0)
	{
		float dist = texture(shadowMapSampler, vec3(coord.xy + offset, index)).r;
		float bias = getShadowBias(0.4, 0.0);
		if (dist < coord.z)
			shadow = 0.0;
	}
	
	return shadow;
}

float filterPCF3x3(vec3 coords, uint index)
{	
	ivec2 texDim = textureSize(shadowMapSampler, 0).xy;
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(coords, vec2(dx*x, dy*y), index);
			count++;
		}
	
	}
	return shadowFactor / count;
}

float pcf7x7(vec3 texCoord, uint index)
{
	ivec2 itexelSize = textureSize(shadowMapSampler, 0).xy;

	float deltaSize = 1.0 / float(itexelSize.x);
	vec2 texelSize= vec2(deltaSize, deltaSize);

    float depth = 0.0;
    int numSamples = 0;
	int range = 3;

    for (int i = -range; i <= range; i++)
    {
        for (int j = -range; j <= range; j++)
        {
            vec2 offset = vec2(i, j) * texelSize;
            depth += textureProj(texCoord, offset, index);
            numSamples++;
        }
    }

    // 计算平均值
    depth = depth / float(numSamples);

    return depth;
}

float PCF(vec3 coords, float filteringSize, uint index) {
	float bias = 0.0;
	if(ubo.shadowIndex == 2)
		uniformDiskSamples(coords.xy);
	else if(ubo.shadowIndex == 3)
		poissonDiskSamples(coords.xy);

	float shadow = 0.0;
	for(int i = 0; i < NUM_SAMPLES; i ++)
		shadow += textureProj(coords, poissonDisk[i] * filteringSize, index);

	return shadow / float(NUM_SAMPLES);
}

float findBlocker(vec2 coords, float zReceiver, uint index)
{
	int blockerNum = 0;
	float blockDepth = 0.;

	float posZFromLight = zReceiver;// shadowCoord.z;

	float searchRadius = LIGHT_SIZE_UV * (posZFromLight - NEAR_PLANE) / posZFromLight;

	ivec2 itexelSize = textureSize(shadowMapSampler, 0).xy;
	float deltaSize = 1.0 / float(itexelSize.x);
	float filterSize = 1.0;
	float filteringRange = deltaSize * filterSize;

	poissonDiskSamples(coords);
	for(int i = 0; i < NUM_SAMPLES; i++){
		float shadowDepth = texture(shadowMapSampler, vec3(coords + poissonDisk[i] * filteringRange, index)).r;
		if(zReceiver > shadowDepth){
			blockerNum++;
			blockDepth += shadowDepth;
		}
	}

	if(blockerNum == 0)
		return -1.;
	else
		return blockDepth / float(blockerNum);
}

float PCSS(vec3 coords, uint index)
{
	float blockerDepth = findBlocker(coords.xy, coords.z, index);
	if(blockerDepth < -EPS)
		return 1.0;

	float wp = (coords.z - blockerDepth) * LIGHT_WORLD_SIZE / blockerDepth;

	return PCF(coords, wp, index);
}

float getShadow(vec3 coord, uint index)
{
	
	switch(ubo.shadowIndex)
	{
		case 0:
			return textureProj(coord, vec2(0.0), index);
		case 1:
			return filterPCF3x3(coord, index);
		case 2:
		case 3:
			ivec2 texDim = textureSize(shadowMapSampler, 0).xy;
			float filteringSize = ubo.filterSize / float(texDim.x);
			return PCF(coord, filteringSize, index);
		case 4:
			return PCSS(coord, index);
	}
}

/*-----------------shadow-----------------*/

void main(){
	//vec3 color = blin_phong();
	vec3 color = vec3(1.0);

	if(material.islight == 0)
		color = pbr();

	int cascadedIndex = 0;
	for(int i = 0; i < CASCADED_COUNT; i ++)
	{
		if(viewPos.z < ubo.splitDepth[i])
			cascadedIndex ++;
	}

	vec4 shadowCoord = ubo.depthVP[cascadedIndex] * worldPos;

	vec3 coord = shadowCoord.xyz;
	if(shadowCoord.w > 0.0)
	{
		coord = coord / shadowCoord.w;
		coord.xy = coord.xy * 0.5 + 0.5;
	}

	float shadow = getShadow(coord, cascadedIndex);
	color *= shadow;

	outColor = vec4(color, 1.0);
}