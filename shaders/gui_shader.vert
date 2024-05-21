#version 450

vec2 offsets[6] = vec2[](
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

layout (push_constant) uniform PushConstants
{
	vec2 window_size;
	vec2 position;
	vec2 size;
} pc;

layout (location = 0) out vec2 outUV;

void main()
{
	vec2 pos = pc.position / pc.window_size * 2.0 - 1.0;
	vec2 size = pc.size / pc.window_size * 2.0;
	gl_Position = vec4(pos + offsets[gl_VertexIndex] * size, 0.0, 1.0);
	outUV = texCoords[gl_VertexIndex];
}
