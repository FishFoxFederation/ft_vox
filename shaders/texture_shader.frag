#version 450

layout(set = 0, binding = 0) uniform sampler2D shadowMap;
layout(set = 1, binding = 0) uniform sampler2D depthMap;
layout(set = 2, binding = 0) uniform sampler2D colorMap;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	float color = texture(colorMap, fragTexCoord).r;
    outColor = vec4(color, color, color, 1.0);
}
