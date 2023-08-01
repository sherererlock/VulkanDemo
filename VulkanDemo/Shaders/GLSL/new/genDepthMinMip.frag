#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform sampler2D uDepthMipMap;

layout(set = 0, binding = 1)
uniform MipData{
	vec2 uLastMipSize;
	int currentMipLv;
} mipData;

layout (location = 0) in vec2 inUV;

layout(location = 0) out vec4 outDepth;

void main() 
{

    ivec2 thisLevelTexelCoord = ivec2(gl_FragCoord);
    ivec2 previousLevelBaseTexelCoord = thisLevelTexelCoord * 2;

    vec4 depthTexelValues;
    depthTexelValues.x = texelFetch(uDepthMipMap,
                                      previousLevelBaseTexelCoord,
                                      0).r;
    depthTexelValues.y = texelFetch(uDepthMipMap,
                                      previousLevelBaseTexelCoord + ivec2(1, 0),
                                      0).r;
    depthTexelValues.z = texelFetch(uDepthMipMap,
                                      previousLevelBaseTexelCoord + ivec2(1, 1),
                                      0).r;
    depthTexelValues.w = texelFetch(uDepthMipMap,
                                      previousLevelBaseTexelCoord + ivec2(0, 1),
                                      0).r;

    float minDepth = min(min(depthTexelValues.x, depthTexelValues.y),
                          min(depthTexelValues.z, depthTexelValues.w));

    // Incorporate additional texels if the previous level's width or height (or both)
    // are odd.
    ivec2 u_previousLevelDimensions = ivec2(mipData.uLastMipSize.x, mipData.uLastMipSize.y);

    bool shouldIncludeExtraColumnFromPreviousLevel = ((u_previousLevelDimensions.x & 1) != 0);

    bool shouldIncludeExtraRowFromPreviousLevel = ((u_previousLevelDimensions.y & 1) != 0);

    if (shouldIncludeExtraColumnFromPreviousLevel) {
      vec2 extraColumnTexelValues;
      extraColumnTexelValues.x = texelFetch(uDepthMipMap,
                                                previousLevelBaseTexelCoord + ivec2(2, 0),
                                                0).r;
      extraColumnTexelValues.y = texelFetch(uDepthMipMap,
                                                previousLevelBaseTexelCoord + ivec2(2, 1),
                                                0).r;

      // In the case where the width and height are both odd, need to include the
          // 'corner' value as well.
      if (shouldIncludeExtraRowFromPreviousLevel) {
        float cornerTexelValue = texelFetch(uDepthMipMap,
                                                  previousLevelBaseTexelCoord + ivec2(2, 2),
                                                  0).r;
        minDepth = min(minDepth, cornerTexelValue);
      }
      minDepth = min(minDepth, min(extraColumnTexelValues.x, extraColumnTexelValues.y));
    }
    if (shouldIncludeExtraRowFromPreviousLevel) {
      vec2 extraRowTexelValues;
      extraRowTexelValues.x = texelFetch(uDepthMipMap,
                                            previousLevelBaseTexelCoord + ivec2(0, 2),
                                            0).r;
      extraRowTexelValues.y = texelFetch(uDepthMipMap,
                                            previousLevelBaseTexelCoord + ivec2(1, 2),
                                            0).r;
      minDepth = min(minDepth, min(extraRowTexelValues.x, extraRowTexelValues.y));
    }

    outDepth = vec4(vec3(minDepth), 1.0);		
}