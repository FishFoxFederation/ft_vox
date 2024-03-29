#version 450

layout(set = 0, binding = 0) uniform CameraMatrices
{
	mat4 view;
	mat4 projection;
}cm;

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColors;

void main()
{
    gl_Position = cm.projection * cm.view * vec4(positions, 1.0);
    fragColors = color;
}
