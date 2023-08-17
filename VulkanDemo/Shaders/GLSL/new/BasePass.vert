#version 450
#extension GL_ARB_separate_shader_objects : enable

#define TAA_SAMPLE_COUNT 32

layout(set = 0, binding = 0)
uniform UniformBufferObject{
    mat4 view;
    mat4 proj;
    vec4 viewPos;
} ubo;

layout(set = 0, binding = 1)
uniform ShadowBufferObject{
    mat4 depthVP;
    vec4 splitDepth;
    vec4 params;
}shadowUbo;

layout(set = 3, binding = 0)
uniform HaltonSequence{
    vec4 hbuffer[TAA_SAMPLE_COUNT];
}haltonSequence;

layout(set = 3, binding = 1)
uniform JitterInfo{
    mat4 preViewProj;
    mat4 model;
    uint hindex;
    float width;
    float height;
}jitterInfo;

layout(push_constant) uniform PushConsts{
    mat4 model;
}primitive;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 texcoord;
layout(location = 3) out vec3 tangent;
layout(location = 4) out float vDepth;
layout(location = 5) out vec4 newPos;
layout(location = 6) out vec4 oldPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 position = primitive.model * inPosition;

    mat4 jitterMat = ubo.proj;
    jitterMat[0][2] += (haltonSequence.hbuffer[jitterInfo.hindex].x * 2.0f - 1.0f) / jitterInfo.width;
    jitterMat[1][2] += (haltonSequence.hbuffer[jitterInfo.hindex].y * 2.0f - 1.0f) / jitterInfo.height;

    gl_Position = jitterMat * ubo.view * position;

    worldPos = position.xyz;

    mat3 model = transpose(inverse(mat3(primitive.model)));

    normal = model * inNormal;
    tangent = model * inTangent;

    texcoord = inTexCoord;
    vDepth = gl_Position.w;

    newPos = (ubo.proj * ubo.view * position);
    oldPos = (jitterInfo.preViewProj * position);
}