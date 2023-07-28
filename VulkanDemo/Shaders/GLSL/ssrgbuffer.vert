#version 450
#extension GL_ARB_separate_shader_objects : enable

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

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 position = primitive.model * inPosition;
    gl_Position = ubo.proj * ubo.view * position;

    worldPos = position.xyz;
    mat3 model = transpose(inverse(mat3(primitive.model)));

    normal = model * inNormal;
    tangent = model * inTangent;

    texcoord = inTexCoord;
    vDepth = gl_Position.w;
}