#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) 
uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 viewPos;
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


out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    //gl_Position = vec4(inPosition, 1.0);
   
   gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition, 1.0);

   fragColor = inColor;
   fragTexCoord = inTexCoord;
   normal = mat3(primitive.model) * inNormal;
   tangent = mat3(primitive.model) * inTangent;

   vec4 pos = primitive.model * vec4(inPosition, 1.0);
   worldPos = pos.xyz;

}