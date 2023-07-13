#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define CASCADED_COUNT 1

#include "VertexInput.hlsl"
layout(location = 5) out vec4 outShadowCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 pos = primitive.model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
    mat3 model = mat3(primitive.model);
    normal = model * inNormal;
    tangent = model * inTangent;
    worldPos = pos.xyz;
    outShadowCoord = ubo.depthVP * pos;

    gl_Position = ubo.proj * ubo.view * pos;
}