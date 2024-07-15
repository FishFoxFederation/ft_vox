#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
// #extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct ObjectData
{
	uint64_t vertexBuffer;
	uint64_t indexBuffer;
};

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 uv;
	uint32_t texLayer;
	uint8_t ao;
};

layout(buffer_reference, scalar) buffer VertexBuffer { Vertex vertices[]; };
layout(buffer_reference, scalar) buffer IndexBuffer { ivec3 indices[]; };

layout(set = 1, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = 1, scalar) buffer ObjectBuffer { ObjectData objects[]; };
layout(set = 3, binding = 0) uniform sampler2DArray tex;
layout(set = 4, binding = 0) uniform AtmospherParams
{
	vec3 sun_dir;
	float earth_radius;
	float atmosphere_radius;
	float player_height;
	float h_rayleigh;
	float h_mie;
	float g;
	float sun_intensity;
	float n_samples;
	float n_light_samples;
	vec3 beta_rayleigh;
	vec3 beta_mie;
}ap;

layout(location = 0) rayPayloadInEXT struct RayPayload
{
	vec3 ray_direction;
	vec4 color;
} payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec2 attribs;

void main()
{
	ObjectData object = objects[gl_InstanceCustomIndexEXT];

	IndexBuffer indexBuffer = IndexBuffer(object.indexBuffer);
	VertexBuffer vertexBuffer = VertexBuffer(object.vertexBuffer);

	ivec3 index = indexBuffer.indices[gl_PrimitiveID];

	Vertex v0 = vertexBuffer.vertices[index.x];
	Vertex v1 = vertexBuffer.vertices[index.y];
	Vertex v2 = vertexBuffer.vertices[index.z];

	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	const vec3 pos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
	const vec3 world_pos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));

	const vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
	const vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));

	const vec2 uv = v0.uv * barycentrics.x + v1.uv * barycentrics.y + v2.uv * barycentrics.z;

	float ao = float(v0.ao) * barycentrics.x + float(v1.ao) * barycentrics.y + float(v2.ao) * barycentrics.z;
	ao = 1.0 - (ao / 3.0) * 0.9;

	float attenuation = 0;
	float max_attenuation = 0.7;

	// Tracing shadow ray only if the light is visible from the surface
	if(dot(normal, ap.sun_dir) > 0)
	{
		float tMin   = 0.001;
		float tMax   = 1000;
		vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

		const uint ray_count = 1;
		uint hit_count = 0;

		// create ray_count rays in a circle around the sun direction at angle theta
		vec3 rayDirs[ray_count];
		rayDirs[0] = ap.sun_dir;

		// alpha is the horizontal angle from the X axis
		// polar is the vertical angle from the Z axis
		float alpha = atan(ap.sun_dir.y, ap.sun_dir.x);
		float polar = acos(ap.sun_dir.z);
		const float pi = 3.14159265359;
		const float theta = 1 * pi / 180;
		for (uint i = 1; i < ray_count; i++)
		{
			float alpha_offset = (i * theta) - (ray_count / 2) * theta;
			float polar_offset = (i * theta) - (ray_count / 2) * theta;

			rayDirs[i].x = cos(alpha + alpha_offset) * sin(polar + polar_offset);
			rayDirs[i].y = sin(alpha + alpha_offset) * sin(polar + polar_offset);
			rayDirs[i].z = cos(polar + polar_offset);
		}


		for (uint i = 0; i < ray_count; i++)
		{
			isShadowed = true;
			traceRayEXT(
				topLevelAS,	// acceleration structure
				flags,		// rayFlags
				0xFF,		// cullMask
				0,			// sbtRecordOffset
				0,			// sbtRecordStride
				1,			// missIndex
				origin,		// ray origin
				tMin,		// ray min range
				rayDirs[i],	// ray direction
				tMax,		// ray max range
				1			// payload (location = 1)
			);

			if(isShadowed)
			{
				hit_count++;
			}
		}

		attenuation = (float(hit_count) / float(ray_count)) * max_attenuation;

	}
	else
	{
		attenuation = max_attenuation;
	}

	payload.color = texture(tex, vec3(uv, v0.texLayer)) * ao * (1 - attenuation);
}
