#version 450

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions, 1.0);
    fragColor = normal;
}