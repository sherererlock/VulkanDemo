#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) 
uniform UniformBufferObject
{
	mat4 view;
    mat4 projection;
    vec4 clipPlane;
} ubo;

layout(push_constant) uniform PushConsts{
    mat4 model;
}primitive;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 viewPos;
layout(location = 1) out vec3 viewNormal;
layout(location = 2) out vec2 texcoord;
layout(location = 3) out vec3 viewTangent;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 position = primitive.model * inPosition;
    gl_Position = ubo.projection * ubo.view * position;
    viewPos = vec3(ubo.view * position);
    mat3 normalMatrix = transpose(inverse(mat3(ubo.view * primitive.model)));
    viewNormal = normalMatrix * inNormal;
    viewTangent = normalMatrix * inTangent;
    texcoord = inTexCoord;
}