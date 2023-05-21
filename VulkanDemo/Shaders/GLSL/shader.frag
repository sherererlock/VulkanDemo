#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 lightVec;
layout(location = 4) in vec3 viewVec;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    
	vec4 color = texture(texSampler, fragTexCoord) * vec4(fragColor, 1.0);

	vec3 N = normalize(normal);
	vec3 L = normalize(lightVec);
	vec3 V = normalize(viewVec);
	
	vec3 R = reflect(L, N);
	vec3 diffuse = max(dot(N, L), 0.15) * fragColor;
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
	outColor = vec4(diffuse * color.rgb + specular, 1.0);
}