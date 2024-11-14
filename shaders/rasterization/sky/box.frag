#version 460

#include "common.glsl"

layout(set = 0, binding = SKYBOX_CUBE_MAP_BINDING) uniform samplerCube cubeMapTexture;

layout(location = 0) in vec3 texDir;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(cubeMapTexture, texDir);
}
