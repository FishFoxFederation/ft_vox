#version 450

layout(set=0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform PushConstantObject {
	mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
	
	// convert normal to world space
	fragNormal = normalize((pc.model * vec4(inNormal, 0.0)).xyz);

	fragTexCoord = inTexCoord;
}
