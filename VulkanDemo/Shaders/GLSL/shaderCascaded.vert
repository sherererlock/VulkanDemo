#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define CASCADED_COUNT 4

#include "VertexInput.hlsl"
layout(location = 5) out vec3 viewPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {

   worldPos = (primitive.model * vec4(inPosition, 1.0)).xyz;
   viewPos = (ubo.view * vec4(worldPos, 1.0)).xyz;
   gl_Position = ubo.proj * vec4(viewPos, 1.0);

   fragColor = inColor;
   fragTexCoord = inTexCoord;
   normal = mat3(primitive.model) * inNormal;
   tangent = mat3(primitive.model) * inTangent;
}