#ifndef	SHADOW
#define SHADOW

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
	ivec2 texDim = textureSize(shadowMapSampler, 0);
	vec3 lightDir = normalize(uboParam.lights[0].xyz - worldPos);
	vec3 normal = normalize(normal);
	float m = FRUSTUM_SIZE / float(texDim.x) / 2.0; //�����������/shadowMapSize/2
	float bias = max(m, m * (1.0 - dot(normal, lightDir))) * depthCtr;

	return bias;
}

float getShadowBias(float c, float filterRadiusUV){
	ivec2 texDim = textureSize(shadowMapSampler, 0);
	vec3 normal = normalize(normal);
	vec3 lightDir = normalize(uboParam.lights[0].xyz - worldPos);
	float fragSize = (1. + ceil(filterRadiusUV)) * (FRUSTUM_SIZE / texDim.x / 2.);
	return max(fragSize, fragSize * (1.0 - dot(normal, lightDir))) * c;
}

float textureProj(vec3 coord, vec2 offset)
{
	float shadow = 1.0;

	if(coord.z > -1.0 && coord.z < 1.0)
	{
		float dist = texture(shadowMapSampler, coord.xy + offset).r;
		if (dist < coord.z)
			shadow = 0.1;
	}
	
	return shadow;
}

float filterPCF3x3(vec3 coords)
{	
	ivec2 texDim = textureSize(shadowMapSampler, 0);
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
			shadowFactor += textureProj(coords, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

float pcf7x7(vec3 texCoord)
{
	ivec2 itexelSize = textureSize(shadowMapSampler, 0);

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
            depth += textureProj(texCoord, offset);
            numSamples++;
        }
    }

    // ����ƽ��ֵ
    depth = depth / float(numSamples);

    return depth;
}

float PCF(vec3 coords, float filteringSize) {
	float bias = 0.0;
	int shadowIndex = int(shadowUbo.params.x);
	if(shadowIndex == 2)
		uniformDiskSamples(coords.xy);
	else if(shadowIndex == 3)
		poissonDiskSamples(coords.xy);

	float shadow = 0.0;
	for(int i = 0; i < NUM_SAMPLES; i ++)
		shadow += textureProj(coords, poissonDisk[i] * filteringSize);

	return shadow / float(NUM_SAMPLES);
}

float findBlocker(vec2 coords, float zReceiver)
{
	int blockerNum = 0;
	float blockDepth = 0.;

	float posZFromLight = shadowCoord.z;

	float searchRadius = LIGHT_SIZE_UV * (posZFromLight - NEAR_PLANE) / posZFromLight;

	ivec2 itexelSize = textureSize(shadowMapSampler, 0);
	float deltaSize = 1.0 / float(itexelSize.x);

	float filteringRange = deltaSize * searchRadius;

	poissonDiskSamples(coords);
	for(int i = 0; i < NUM_SAMPLES; i++){
		float shadowDepth = texture(shadowMapSampler, coords + poissonDisk[i] * filteringRange).r;
		if(zReceiver > shadowDepth){
			blockerNum++;
			blockDepth += shadowDepth;
		}
	}

	if (blockerNum == 0)
		return 1.0;

	if (blockerNum == NUM_SAMPLES)
		return -1.0;

	float avgDepth = blockDepth / float(blockerNum);

	return avgDepth;
}

float findBlocker2(vec2 uv, float zReceiver) {

	poissonDiskSamples(uv);
	int blockerCount = 0;
	float blockerDepth = 0.0;

	ivec2 itexelSize = textureSize(shadowMapSampler, 0);
	float textureSize = float(itexelSize.x);
	float filterSize = 20.0;
	float filteringRange = filterSize / textureSize;

	for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; i++)
	{
		vec2 coords = uv + poissonDisk[i] * filteringRange;
		float depth = texture(shadowMapSampler, coords).r;
		if (zReceiver > depth)
		{
			blockerDepth += depth;
			blockerCount++;
		}
	}

	if (blockerCount == 0)
		return 1.0;

	if (blockerCount == BLOCKER_SEARCH_NUM_SAMPLES)
		return -1.0;

	float avgDepth = blockerDepth / float(blockerCount);

	return avgDepth;
}

#define Light_width 20.0
float PCSS(vec3 coords)
{
	float blockerDepth = findBlocker2(coords.xy, coords.z);
	if(blockerDepth < 0.0)
		return 0.0;

	if (blockerDepth == 1.0)
		return 1.0;

	float wp = (coords.z - blockerDepth) * Light_width / blockerDepth;

	//float wp = (coords.z - blockerDepth) * LIGHT_SIZE_UV / blockerDepth;

	ivec2 itexelSize = textureSize(shadowMapSampler, 0);

	float textureSize = itexelSize.x;

	float filteringSize = wp * 1.0 / textureSize;

	return PCF(coords, filteringSize);
}

float getShadow(vec3 coord)
{
	int shadowIndex = int(shadowUbo.params.x);
	switch(shadowIndex)
	{
		case 0:
			return textureProj(coord, vec2(0.0));
		case 1:
			return filterPCF3x3(coord);
		case 2:
		case 3:
			ivec2 texDim = textureSize(shadowMapSampler, 0);
			float filteringSize = shadowUbo.params.y / float(texDim.x);
			return PCF(coord, filteringSize);
		case 4:
			return PCSS(coord);
	}
}

#endif