#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	float color = texture(tex, fragTexCoord).r;
    outColor = vec4(color, color, color, 1.0);
}
