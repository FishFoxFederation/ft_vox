#version 450

// Vertex shader for the skybox

layout(set = 0, binding = 0) uniform CameraMatrices
{
	mat4 view;
	mat4 projection;
}cm;

vec3 skyboxVertices[8] = vec3[8](
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3( 1.0, -1.0,  1.0),
	vec3( 1.0,  1.0,  1.0)
);

layout(location = 0) out vec3 TexCoords;

void main()
{
	TexCoords = skyboxVertices[gl_VertexID];
	gl_Position = projection * view * vec4(skyboxVertices[gl_VertexID], 1.0);
}
