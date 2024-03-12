#version 450

layout(set = 1, binding = 0) uniform samplerCube cubeMapTexture;

layout(location = 0) in vec3 texDir;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(cubeMapTexture, texDir);
	// outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
