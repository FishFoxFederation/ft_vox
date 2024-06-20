#version 450

// Vertex shader for the skybox

layout(set = 0, binding = 0) uniform CameraMatrices
{
	mat4 view;
	mat4 projection;
}cm;

layout(push_constant) uniform PushConstants
{
	mat4 model;
}pc;

vec3 skyboxVertices[36] = vec3[36](
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),

	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),

	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),

	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0, -1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),

	vec3(-1.0,  1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0,  1.0, -1.0),

	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3( 1.0, -1.0,  1.0)
);

layout(location = 0) out vec3 texDir;

void main()
{
	texDir = vec3(skyboxVertices[gl_VertexIndex]);
	vec4 pos = cm.projection * cm.view * pc.model * vec4(skyboxVertices[gl_VertexIndex], 1.0);
	gl_Position = pos.xyww;
}
