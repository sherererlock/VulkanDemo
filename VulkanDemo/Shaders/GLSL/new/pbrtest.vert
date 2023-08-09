#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0)
uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 viewPos;
    vec4 lightPos[4];
} ubo;

layout(push_constant) uniform PushConsts{
    vec3 objPos;
}primitive;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec4 position;
layout(location = 1) out vec3 normal;

void main() {
    vec4 pos = ubo.model * inPosition;
    pos.xyz += primitive.objPos;
    normal = mat3(ubo.model) * inNormal;

    gl_Position = ubo.proj * ubo.view * pos;
}