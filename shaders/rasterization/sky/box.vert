#version 450

#include "common.glsl"

layout(set = 0, binding = 0) uniform CameraMatrices
{
	ViewProjMatrices cm;
};

layout(push_constant) uniform PushConstants
{
	ModelMatrice pc;
};

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
	vec4 pos = cm.proj * cm.view * pc.model * vec4(skyboxVertices[gl_VertexIndex], 1.0);
	gl_Position = pos.xyww;
}
