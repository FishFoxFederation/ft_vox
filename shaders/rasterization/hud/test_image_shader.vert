#version 450

#include "common.glsl"

vec2 positions[6] = vec2[](
	vec2( 1.0, -1.0),
	vec2(-1.0, -1.0),
	vec2( 1.0,  1.0),
	vec2( 1.0,  1.0),
	vec2(-1.0, -1.0),
	vec2(-1.0,  1.0)
);

vec2 texCoords[6] = vec2[](
	vec2(1.0, 0.0),
	vec2(0.0, 0.0),
	vec2(1.0, 1.0),
	vec2(1.0, 1.0),
	vec2(0.0, 0.0),
	vec2(0.0, 1.0)
);

layout (location = 0) out vec2 outUV;

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	outUV = texCoords[gl_VertexIndex];
}
