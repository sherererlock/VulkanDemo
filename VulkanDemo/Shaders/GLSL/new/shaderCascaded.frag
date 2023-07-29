#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include"macros.hlsl"

#define CASCADED_COUNT 4
#include "FragmentInput.hlsl"
layout(set = 1, binding = 1) uniform sampler2DArray shadowMapSampler;

layout(location = 5) in vec3 viewPos;

vec2 GetRoughnessAndMetallic()
{
    vec2 roughMetalic = texture(roughnessSampler, fragTexCoord).gb;
	return roughMetalic;
}

#include "lighting.hlsl"

layout(location = 0) out vec4 outColor;

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
	vec3 lightDir = normalize(uboParam.lights[0].xyz - worldPos);
	vec3 normal = normalize(normal);
	float m = FRUSTUM_SIZE / float(texDim.x) / 2.0; //�����������/shadowMapSize/2
	float bias = max(m, m * (1.0 - dot(normal, lightDir))) * depthCtr;

	return bias;
}

float getShadowBias(float c, float filterRadiusUV){
	ivec2 texDim = textureSize(shadowMapSampler, 0).xy;
	vec3 normal = normalize(normal);
	vec3 lightDir = normalize(uboParam.lights[0].xyz - worldPos);
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

    // ����ƽ��ֵ
    depth = depth / float(numSamples);

    return depth;
}

float PCF(vec3 coords, float filteringSize, uint index) {
	float bias = 0.0;
	
	int shadowIndex = int(shadowUbo.params.x);
	if(shadowIndex == 2)
		uniformDiskSamples(coords.xy);
	else if(shadowIndex == 3)
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
	int shadowIndex = int(shadowUbo.params.x);
	switch(shadowIndex)
	{
		case 0:
			return textureProj(coord, vec2(0.0), index);
		case 1:
			return filterPCF3x3(coord, index);
		case 2:
		case 3:
			ivec2 texDim = textureSize(shadowMapSampler, 0).xy;
			float filteringSize = shadowUbo.params.y / float(texDim.x);
			return PCF(coord, filteringSize, index);
		case 4:
			return PCSS(coord, index);
	}
}

/*-----------------shadow-----------------*/

void main(){
	//vec3 color = blin_phong();
	vec3 color = vec3(1.0);

	int cascadedIndex = 0;
	for(int i = 0; i < CASCADED_COUNT; i ++)
	{
		if(viewPos.z < shadowUbo.splitDepth[i])
			cascadedIndex ++;
	}

	vec4 shadowCoord = shadowUbo.depthVP[cascadedIndex] * vec4(worldPos, 1.0);

	vec3 coord = shadowCoord.xyz;
	if(shadowCoord.w > 0.0)
	{
		coord = coord / shadowCoord.w;
		coord.xy = coord.xy * 0.5 + 0.5;
	}

	float shadow = getShadow(coord, cascadedIndex);
	color = Lighting(shadow);

	color = pow(color, vec3(1.0/2.2));
	//outColor = vec4(viewPos.z / (-32.0), 0.0,0.0, 1.0);
	outColor = vec4(color, 1.0);

	if (shadowUbo.params.z == 1.0) {
		switch(cascadedIndex) {
			case 0 : 
				outColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
				break;
			case 1 : 
				outColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
				break;
			case 2 : 
				outColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
				break;
			case 3 : 
				outColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
				break;
		}
	}
}