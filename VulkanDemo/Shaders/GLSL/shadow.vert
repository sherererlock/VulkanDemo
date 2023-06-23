#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) 
uniform UniformBufferObject {
    mat4 lightMVP;
} ubo;

layout(location = 0) in vec3 inPosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
   gl_Position = ubo.lightMVP * vec4(inPosition, 1.0);
}