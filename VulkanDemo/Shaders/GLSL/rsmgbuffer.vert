#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) 
uniform UniformBufferObject
{
	float nearPlane;
	float farPlane;
	vec3 lightPos;
	vec3 lightColor;
	mat4 depthVP;
} ubo;

layout(push_constant) uniform PushConsts{
    mat4 model;
}primitive;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 texcoord;
layout(location = 3) out vec3 tangent;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 position = primitive.model * vec4(inPosition, 1.0);
    gl_Position = ubo.depthVP * position;
    worldPos = position.xyz;
    mat3 model = mat3(primitive.model);
    normal = model * inNormal;
    tangent = model * inTangent;
    texcoord = inTexCoord;
}