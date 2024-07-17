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
layout(set = 2, binding = 0) uniform Camera
{
	mat4 view;
	mat4 proj;
	mat4 last_view;
	mat4 last_proj;
} camera;
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
	float shadow;
	float depth;
	vec2 motion;
} payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec2 attribs;

layout(push_constant) uniform PushConstant
{
	float time;
} pc;

// float unlinearize_depth(float d,float n,float f)
// {
// 	return f * (d - n) / d * (f - n);
// }

float random_seed = pc.time * gl_LaunchIDEXT.x * gl_LaunchIDEXT.y;

uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }

float random()
{
	random_seed  = random(random_seed);
	return random_seed;
}

bool equal_float(float a, float b)
{
	return abs(a - b) < 0.0001;
}

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


	float light_distance = 1000.0;
	vec3 to_light = normalize(ap.sun_dir);
	vec3 light_pos = world_pos + to_light * light_distance;
	float light_size = 10.0;

	// vectors to the side of the light
	vec3 perpL = cross(to_light, vec3(0, 1, 0));
	if (perpL == vec3(0.0))
	{
		perpL.x = 1.0;
	}
	vec3 perpL2 = cross(to_light, perpL);
	// the light is a square with a random offset
	vec2 light_offset = (vec2(random(), random()) * 2.0 - 1.0) * light_size;
	vec3 new_light_pos = light_pos + perpL * light_offset.x + perpL2 * light_offset.y;
	vec3 ray_dir = normalize(new_light_pos - world_pos);


	float attenuation = 1.0;

	// Tracing shadow ray only if the light is visible from the surface
	if (dot(normal, to_light) > 0)
	{
		float tMin = 0.001;
		float tMax = light_distance;
		vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

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
			ray_dir,	// ray direction
			tMax,		// ray max range
			1			// payload (location = 1)
		);

		if (isShadowed)
		{
			attenuation = 0.3;
		}
	}
	else
	{
		attenuation = 0.3;
	}


	vec4 clip_pos = camera.proj * camera.view * gl_ObjectToWorld3x4EXT * pos;
	vec2 ndc = clip_pos.xy / clip_pos.w;
	// Wrong because this is the world_pos of the current frame (but it works for non-moving objects)
	vec4 last_clip_pos = camera.last_proj * camera.last_view * gl_ObjectToWorld3x4EXT * pos;
	vec2 last_ndc = last_clip_pos.xy / last_clip_pos.w;
	vec2 motion = (ndc - last_ndc);

	payload.color = texture(tex, vec3(uv, v0.texLayer)) * ao;
	payload.shadow = attenuation;
	// payload.depth = unlinearize_depth(gl_HitTEXT, 0.001, 1000);
	payload.depth = sqrt(gl_HitTEXT) / 10.0;
	// payload.motion = motion;
}

