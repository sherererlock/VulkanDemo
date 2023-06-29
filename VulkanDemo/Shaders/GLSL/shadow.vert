#version 450
#extension GL_ARB_separate_shader_objects : enable

#define CASCADED_COUNT 4
layout(set = 0, binding = 0) 
uniform UniformBufferObject {
    mat4 depthVP[CASCADED_COUNT];
    uint casecadedIndex;
} ubo;

layout(push_constant) uniform PushConsts{
    mat4 model;
}primitive;

layout(location = 0) in vec3 inPosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
   gl_Position = ubo.depthVP[ubo.casecadedIndex] * primitive.model * vec4(inPosition, 1.0);
}