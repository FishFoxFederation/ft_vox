#version 450

vec2 screenCoords[6] = vec2[6](
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
);

vec2 texCoords[6] = vec2[6](
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0)
);

layout(location = 0) out vec2 fragTexCoords;

void main()
{
	gl_Position = vec4(screenCoords[gl_VertexIndex], 0.0, 1.0);
	fragTexCoords = texCoords[gl_VertexIndex];
}
