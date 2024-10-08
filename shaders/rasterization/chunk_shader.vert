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

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in uint texLayer;
layout(location = 4) in uint ao;
layout(location = 5) in uint light;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragTexCoords;
layout(location = 2) out vec4 fragPosWorldSpace;
layout(location = 3) out float fragAO;
layout(location = 4) out float fragSkyLight;
layout(location = 5) out float fragBlockLight;

void main()
{
	gl_Position = cm.proj * cm.view * pc.model * vec4(positions, 1.0);

	fragNormal = normal;
	fragTexCoords = vec3(texCoords, texLayer);

	fragPosWorldSpace = pc.model * vec4(positions, 1.0);

	fragAO = float(ao);
	fragSkyLight = float(light & 0x0F);
	fragBlockLight = float(light >> 4 & 0x0F);
}
