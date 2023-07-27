#ifndef	VINPUT
#define VINPUT

layout(set = 0, binding = 0)
uniform UniformBufferObject{
    mat4 view;
    mat4 proj;
    mat4 depthVP;
    vec4 splitDepth;
    vec4 viewPos;
    int shadowIndex;
    float filterSize;
} ubo;

layout(push_constant) uniform PushConsts{
    mat4 model;
}primitive;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 worldPos;
layout(location = 4) out vec3 tangent;


#endif