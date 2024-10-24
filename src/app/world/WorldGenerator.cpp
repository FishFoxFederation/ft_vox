#include "WorldGenerator.hpp"
#include "World.hpp"

#include "math_utils.hpp"
#include <cmath>
#include <queue>

// Chunk::genLevel World::WorldGenerator::ticketToGenLevel(int ticket_level)
// {
// 	if (ticket_level <= TICKET_LEVEL_INACTIVE)
// 		return LIGHT;
// 	switch (ticket_level)
// 	{
// 		case TICKET_LEVEL_INACTIVE:
// 			return LIGHT;
// 		case TICKET_LEVEL_INACTIVE + 1:
// 			return CAVE;
// 			// return RELIEF;
// 		default:
// 			throw std::invalid_argument("World::WorldGenerator::ticketToGenLevel: Invalid ticket level");
// 	}
// }

// World::WorldGenerator::genInfo World::WorldGenerator::getGenInfo(Chunk::genLevel desired_gen_level, Chunk::genLevel old_gen_level, glm::ivec3 chunkPos3D)
// {
// 	genInfo info;

// 	// info.oldLevel = old_gen_level;

// 	switch (desired_gen_level)
// 	{
// 		case LIGHT:
// 		{
// 			genInfo::zone zone;


// 			zone.level = LIGHT;
// 			zone.size = glm::ivec3(1, 0, 1);
// 			zone.start = chunkPos3D;
// 			info.zones.push_back(zone);

// 			zone.level = CAVE;
// 			zone.size = glm::ivec3(3, 0, 3);
// 			zone.start = chunkPos3D - glm::ivec3(1, 0, 1);
// 			info.zones.push_back(zone);
// 			break;
// 		}
// 		case CAVE:
// 		{
// 			genInfo::zone zone;
// 			zone.level = CAVE;
// 			zone.size = glm::ivec3(1, 0, 1);
// 			zone.start = chunkPos3D;
// 			info.zones.push_back(zone);
// 			break;
// 		}
// 		case RELIEF:
// 		{
// 			genInfo::zone zone;

// 			zone.level = RELIEF;
// 			zone.size = glm::ivec3(1, 0, 1);
// 			zone.start = chunkPos3D;
// 			info.zones.push_back(zone);
// 			break;
// 		}
// 		case EMPTY:
// 		{
// 			throw std::invalid_argument("Invalid desired gen level");
// 		}
// 	}

// 	/*
// 		gen level 			zone size
// 		CAVE 				5
// 		RELIEF				10
// 		EMPTY				NULL

// 		if chunk is empty and we need cave level generation,
// 		we need to do it on a relief sized zone because
// 		if a chunk has a relief level generation then we have the garantee
// 		that all the chunks in its zone have at least the same gen level
// 	*/

// 	//if the level difference is more than 1, then we need to change the zone size
// 	// if (static_cast<int>(info.oldLevel) - static_cast<int>(info.level) > 1)
// 		// info.zoneSize = ZONE_SIZES[static_cast<int>(info.oldLevel) - 1];

// 	// info.start = chunkPos3D;
// 	// LOG_INFO("CHUNKPOS: " << chunkPos3D.x << " " << chunkPos3D.z << " SIZE: " << info.zoneSize.x);
// 	//get the start position of the zone the chunk is in
// 	// info.start.y = 0;

// 	// if (info.start.x < 0 && info.zoneSize.x > 1)
// 	// 	info.start.x -= info.zoneSize.x;
// 	// info.start.x -= info.start.x % info.zoneSize.x;
// 	// if (info.start.z < 0 && info.zoneSize.z > 1)
// 	// 	info.start.z -= info.zoneSize.z;
// 	// info.start.z -= info.start.z % info.zoneSize.z;

// 	// LOG_INFO("ZONE START: " << info.start.x << " " << info.start.z);

// 	return info;
// }

World::WorldGenerator::WorldGenerator(World & world)
: m_relief_perlin(1, 7, 1, 0.35, 2),
  m_cave_perlin(1, 4, 1, 0.5, 2),
  m_continentalness_perlin(10, 7, 1, 0.4, 2),
  m_world(world)
{
	// for(size_t i = 0; i + 1 < ZONE_SIZES.size(); i++)
	// {
	// 	if (ZONE_SIZES[i + 1].x % ZONE_SIZES[i].x != 0)
	// 		throw std::logic_error("World::WorldGenerator: constructor: ZONE_SIZES must be multiples of the previous zone size");
	// }
}

World::WorldGenerator::~WorldGenerator()
{

}

BlockInfo::Type World::WorldGenerator::generateReliefBlock(glm::ivec3 position)
{
	float value = generateReliefValue(glm::ivec2(position.x, position.z));

	value *= CHUNK_Y_SIZE;

	if (value > position.y)
		return BlockInfo::Type::Stone;
	// else if (value > -0.05f && value < 0.05f)
		// chunk.setBlock(blockX, blockY, blockZ, BlockInfo::Air);
	else if (value + 3 > position.y)
		return BlockInfo::Type::Grass;
	else
		return BlockInfo::Type::Air;
}

/**
 * @brief
 *
 * @param position
 * @return float [-1, 1]
 */
float World::WorldGenerator::generateReliefValue(glm::ivec2 position)
{
	float value = m_relief_perlin.noise(glm::vec2(
		position.x * 0.0080f,
		position.y * 0.0080f
	));


	// value = (value + 1) / 2;

	// value = pow(2, 10 * value - 10);
	// value = value < 0.5 ? 4 * value * value * value : 1 - pow(-2 * value + 2, 3) / 2;

	// value = pow(2, value);

	// constexpr double slope = 1.0 * (1 - 0) / (2 - 0.5);
	// value = slope * (value - 0.5);
	//value is back to [0, 1]


	return value;
}

BlockInfo::Type World::WorldGenerator::generateCaveBlock(glm::ivec3 position)
{


	//spaghetti caves
	float valueA = m_cave_perlin.noise(glm::vec3(
		position.x * 0.01f,
		position.y * 0.016f,
		position.z * 0.01f
	));

	float valueB = m_cave_perlin.noise(glm::vec3(
		position.x * 0.01f,
		(position.y + 42) * 0.016f,
		position.z * 0.01f
	));

	float threshold = 0.002f;
	float value = valueA * valueA + valueB * valueB;
	// value /= threshold;

	// m_avg += value;
	// if (value > m_max)
	// 	m_max = value;
	// if (value < m_min)
	// 	m_min = value;
	// m_called++;

	if (value < threshold)
		return BlockInfo::Type::Air;

	// else try to create cheese cave

	valueA = (valueA + 1) / 2;
	valueA += 0.01f;

	//bias value to have less chance of a cave the higher the y value
	//for now the transition layer will be between 110 and 128

	float edge = 0.05f;

	if (position.y > 100 && position.y < 140)
		edge = 0.025f;
	else if (position.y >= 140 && position.y < 141)
		edge = 0.02f;
	else if (position.y >= 141 && position.y < 142)
		edge = 0.015f;
	else if (position.y >= 142 && position.y < 143)
		edge = 0.01f;
	else if (position.y >= 143)
		edge = 0;

	if (valueA > 0 && valueA < edge)
		return BlockInfo::Type::Air;
	else
		return BlockInfo::Type::Stone;

	return BlockInfo::Type::Stone;
}

static void setSkyLight(
	ChunkMap & chunks,
	const glm::ivec3 & chunk_pos
)
{
	std::shared_ptr<Chunk> start_chunk = chunks.at(chunk_pos);

	std::queue<glm::ivec3> light_source_queue;
	for (int x = 0; x < CHUNK_X_SIZE; x++)
	{
		for (int z = 0; z < CHUNK_Z_SIZE; z++)
		{
			const glm::ivec3 block_chunk_pos = glm::ivec3(x, CHUNK_Y_SIZE - 1, z);
			const glm::ivec3 block_world_pos = chunk_pos * CHUNK_SIZE_IVEC3 + block_chunk_pos;
			const BlockInfo::Type block_id = start_chunk->getBlock(block_chunk_pos);
			if (!g_blocks_info.hasProperty(block_id, BLOCK_PROPERTY_OPAQUE))
			{
				const int absorbed_light = g_blocks_info.get(block_id).absorb_light;
				const int new_light = 15 - absorbed_light;
				start_chunk->setSkyLight(block_chunk_pos, new_light);
				light_source_queue.push(block_world_pos);
			}
		}
	}

	constexpr std::array<glm::ivec3, 6> NEIGHBOR_OFFSETS = {
		glm::ivec3(1, 0, 0),
		glm::ivec3(-1, 0, 0),
		glm::ivec3(0, 1, 0),
		glm::ivec3(0, -1, 0),
		glm::ivec3(0, 0, 1),
		glm::ivec3(0, 0, -1)
	};

	while (!light_source_queue.empty())
	{
		if (light_source_queue.size() > BLOCKS_PER_CHUNK)
		{
			LOG_WARNING("WorldGenerator: setSkyLight: light_source_queue size > BLOCKS_PER_CHUNK (" << BLOCKS_PER_CHUNK << ")");
			break;
		}

		const glm::ivec3 current_block_world_pos = light_source_queue.front();
		light_source_queue.pop();

		const glm::ivec3 chunk_pos = getChunkPos(current_block_world_pos);
		const glm::ivec3 block_chunk_pos = getBlockChunkPos(current_block_world_pos);
		const std::shared_ptr<Chunk> chunk = chunks.at(chunk_pos);

		// const BlockInfo::Type current_id = chunk->getBlock(block_chunk_pos);
		const int current_light = chunk->getSkyLight(block_chunk_pos);

		if (current_light == 0)
			continue;

		for (int i = 0; i < 6; i++)
		{
			const glm::ivec3 neighbor_world_pos = current_block_world_pos + NEIGHBOR_OFFSETS[i];
			const glm::ivec3 neighbor_chunk_pos = getChunkPos(neighbor_world_pos);
			const glm::ivec3 neighbor_block_chunk_pos = getBlockChunkPos(neighbor_world_pos);

			if (chunks.contains(neighbor_chunk_pos))
			{
				const std::shared_ptr<Chunk> neighbor_chunk = chunks.at(neighbor_chunk_pos);
				const BlockInfo::Type neighbor_id = neighbor_chunk->getBlock(neighbor_block_chunk_pos);
				const int neighbor_light = neighbor_chunk->getSkyLight(neighbor_block_chunk_pos);

				if (!g_blocks_info.hasProperty(neighbor_id, BLOCK_PROPERTY_OPAQUE))
				{
					const int absorbed_light = g_blocks_info.get(neighbor_id).absorb_light;
					const int new_light = current_light - absorbed_light
						- (i != 3 || current_light != 15) * 1; // i == 3 is when the current light spreads down
					if (neighbor_light < new_light)
					{
						neighbor_chunk->setSkyLight(neighbor_block_chunk_pos, new_light);
						light_source_queue.push(neighbor_world_pos);
					}
				}
			}
		}
	}
}

static void setBlockLight(
	ChunkMap & chunks,
	const glm::ivec3 & chunk_pos
)
{
	std::shared_ptr<Chunk> start_chunk = chunks.at(chunk_pos);

	std::queue<glm::ivec3> light_source_queue;
	for (int x = 0; x < CHUNK_X_SIZE; x++)
	{
		for (int y = 0; y < CHUNK_Y_SIZE; y++)
		{
			for (int z = 0; z < CHUNK_Z_SIZE; z++)
			{
				const glm::ivec3 block_chunk_pos = glm::ivec3(x, y, z);
				const glm::ivec3 block_world_pos = chunk_pos * CHUNK_SIZE_IVEC3 + block_chunk_pos;
				const BlockInfo::Type block_id = start_chunk->getBlock(block_chunk_pos);
				if (g_blocks_info.hasProperty(block_id, BLOCK_PROPERTY_LIGHT))
				{
					const int emit_light = g_blocks_info.get(block_id).emit_light;
					start_chunk->setBlockLight(block_chunk_pos, emit_light);
					light_source_queue.push(block_world_pos);
				}
			}
		}
	}

	constexpr std::array<glm::ivec3, 6> NEIGHBOR_OFFSETS = {
		glm::ivec3(1, 0, 0),
		glm::ivec3(-1, 0, 0),
		glm::ivec3(0, 1, 0),
		glm::ivec3(0, -1, 0),
		glm::ivec3(0, 0, 1),
		glm::ivec3(0, 0, -1)
	};

	while (!light_source_queue.empty())
	{
		const glm::ivec3 current_block_world_pos = light_source_queue.front();
		light_source_queue.pop();

		const glm::ivec3 chunk_pos = getChunkPos(current_block_world_pos);
		const glm::ivec3 block_chunk_pos = getBlockChunkPos(current_block_world_pos);
		std::shared_ptr<Chunk> chunk = chunks.at(chunk_pos);

		// const BlockInfo::Type current_id = chunk->getBlock(block_chunk_pos);
		const int current_light = chunk->getBlockLight(block_chunk_pos);

		if (current_light == 0)
			continue;

		for (int i = 0; i < 6; i++)
		{
			const glm::ivec3 neighbor_world_pos = current_block_world_pos + NEIGHBOR_OFFSETS[i];
			const glm::ivec3 neighbor_chunk_pos = getChunkPos(neighbor_world_pos);
			const glm::ivec3 neighbor_block_chunk_pos = getBlockChunkPos(neighbor_world_pos);

			if (chunks.contains(neighbor_chunk_pos))
			{
				std::shared_ptr<Chunk> neighbor_chunk = chunks.at(neighbor_chunk_pos);
				const BlockInfo::Type neighbor_id = neighbor_chunk->getBlock(neighbor_block_chunk_pos);
				const int neighbor_light = neighbor_chunk->getBlockLight(neighbor_block_chunk_pos);

				if (!g_blocks_info.hasProperty(neighbor_id, BLOCK_PROPERTY_OPAQUE))
				{
					const int absorbed_light = g_blocks_info.get(neighbor_id).absorb_light;
					const int new_light = current_light - absorbed_light - 1;
					if (neighbor_light < new_light)
					{
						neighbor_chunk->setBlockLight(neighbor_block_chunk_pos, new_light);
						light_source_queue.push(neighbor_world_pos);
					}
				}
			}
		}
	}
}

// void World::WorldGenerator::generate(genInfo::zone info, ChunkMap & chunkGenGrid)
// {
// 	if (info.level <= RELIEF && info.oldLevel > RELIEF)
// 	{
// 		for(int x = 0; x < info.size.x; x++)
// 		{
// 			for(int z = 0; z < info.size.z; z++)
// 			{
// 				glm::ivec3 chunkPos3D = info.start + glm::ivec3(x, 0, z);
// 				std::shared_ptr<Chunk> chunk = chunkGenGrid.at(chunkPos3D);
// 				std::lock_guard(chunk->status);
// 				chunk->setGenLevel(RELIEF);
// 				for(int blockX = 0; blockX < CHUNK_X_SIZE; blockX++)
// 				{
// 					for(int blockZ = 0; blockZ < CHUNK_Z_SIZE; blockZ++)
// 					{
// 						// check the continentalness of the chunk
// 						float continentalness = m_continentalness_perlin.noise(glm::vec2(
// 							(blockX + chunkPos3D.x * CHUNK_X_SIZE) * 0.00090f,
// 							(blockZ + chunkPos3D.z * CHUNK_Z_SIZE) * 0.00090f
// 						));

// 						// continentalness *= 4; // map from [-1, 1] to [-4, 4]

// 						//generate the relief value for the whole chunk
// 						float reliefValue = generateReliefValue(glm::ivec2(
// 							blockX + chunkPos3D.x * CHUNK_X_SIZE,
// 							blockZ + chunkPos3D.z * CHUNK_Z_SIZE
// 						));

// 						float riverValue = std::abs(reliefValue);

// 						reliefValue = (reliefValue + 1) / 2; // map from [-1, 1] to [0, 1]
// 						reliefValue = pow(2, 10 * reliefValue - 10); // map from [0, 1] to [0, 1] with a slope

// 						float oceanReliefValue = (reliefValue * -60) + 60; // map from [0, 1] to [60, 0]
// 						float landReliefValue = (reliefValue * (CHUNK_Y_SIZE - 80)) + 80; // map from [0, 1] to [80, CHUNK_Y_SIZE]

// 						bool isLand = continentalness > 0.2f;
// 						bool isOcean = continentalness < 0.00f;
// 						bool isCoast = !isLand && !isOcean;

// 						if (isOcean)
// 						{
// 							reliefValue = oceanReliefValue;
// 						}
// 						else if (isCoast)
// 						{
// 							// reliefValue *= (CHUNK_Y_SIZE - 100); // map from [0, 1] to [0, CHUNK_Y_SIZE - 100]
// 							// reliefValue += 100; // map from [0, CHUNK_Y_SIZE - 100] to [100, CHUNK_Y_SIZE]
// 							reliefValue = std::lerp(oceanReliefValue, landReliefValue, mapRange(continentalness, 0.0f, 0.2f, 0.0f, 1.0f));
// 						}
// 						else if (isLand)
// 						{
// 							// reliefValue *= (CHUNK_Y_SIZE - 100); // map from [0, 1] to [0, CHUNK_Y_SIZE - 100]
// 							// reliefValue += 100; // map from [0, CHUNK_Y_SIZE - 100] to [100, CHUNK_Y_SIZE]
// 							reliefValue = landReliefValue;
// 						}

// 						for(int blockY = 0; blockY < CHUNK_Y_SIZE; blockY++)
// 						{

// 							glm::ivec3 position = glm::ivec3(
// 								blockX + chunkPos3D.x * CHUNK_X_SIZE,
// 								blockY,
// 								blockZ + chunkPos3D.z * CHUNK_Z_SIZE
// 							);
// 							BlockInfo::Type to_set = BlockInfo::Type::Air;

// 							{
// 								if (isOcean)
// 								{
// 									if (position.y < reliefValue)
// 										to_set = BlockInfo::Type::Stone;
// 									else if (position.y < 80)
// 										to_set = BlockInfo::Type::Water;
// 									else
// 										to_set = BlockInfo::Type::Air;
// 								}
// 								else if (isCoast)
// 								{
// 									if (position.y > 80)
// 									{
// 										if (position.y < reliefValue - 5)
// 											to_set = BlockInfo::Type::Stone;
// 										else if (position.y < reliefValue - 1 )
// 											to_set = BlockInfo::Type::Dirt;
// 										else if (position.y < reliefValue)
// 											to_set = BlockInfo::Type::Grass;
// 										else
// 											to_set = BlockInfo::Type::Air;
// 									}
// 									else
// 									{
// 										if (position.y < reliefValue)
// 											to_set = BlockInfo::Type::Stone;
// 										else
// 											to_set = BlockInfo::Type::Water;
// 									}
// 								}
// 								else if (isLand)
// 								{
// 									(void)riverValue;
// 									//check to see wether above or below the relief value
// 									// if (reliefValue > position.y)
// 										// to_set = BlockInfo::Type::Stone;
// 									// else if (reliefValue + 5 > position.y)
// 									// {
// 									// 	if (riverValue < 0.05f)
// 									// 		to_set = BlockInfo::Type::Water;
// 									// 	else
// 									// 		to_set = BlockInfo::Type::Grass;
// 									// }
// 									// else
// 										// to_set = BlockInfo::Type::Air;
// 									if (position.y < reliefValue - 5)
// 										to_set = BlockInfo::Type::Stone;
// 									else if (position.y < reliefValue - 1)
// 										to_set = BlockInfo::Type::Dirt;
// 									else if (position.y < reliefValue)
// 										to_set = BlockInfo::Type::Grass;
// 									else
// 										to_set = BlockInfo::Type::Air;
// 								}
// 								// if (isCoast)
// 								// 	to_set = BlockInfo::Type::Glass;
// 							}
// 							chunk->setBlock(blockX, blockY, blockZ, to_set);
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// 	if (info.level <= CAVE && info.oldLevel > CAVE)
// 	{
// 		for(int x = 0; x < info.size.x; x++)
// 		{
// 			for(int z = 0; z < info.size.z; z++)
// 			{
// 				glm::ivec3 chunkPos3D = info.start + glm::ivec3(x, 0, z);
// 				std::shared_ptr<Chunk> chunk = chunkGenGrid.at(chunkPos3D);
// 				std::lock_guard(chunk->status);
// 				chunk->setGenLevel(CAVE);

// 				for(int blockX = 0; blockX < CHUNK_X_SIZE; blockX++)
// 				{
// 					for(int blockZ = 0; blockZ < CHUNK_Z_SIZE; blockZ++)
// 					{
// 						for(int blockY = 0; blockY < CHUNK_Y_SIZE; blockY++)
// 						{

// 							glm::ivec3 position = glm::ivec3(
// 								blockX + chunkPos3D.x * CHUNK_X_SIZE,
// 								blockY,
// 								blockZ + chunkPos3D.z * CHUNK_Z_SIZE
// 							);
// 							BlockInfo::Type current_block = chunk->getBlock(blockX, blockY, blockZ);

// 							if (current_block != BlockInfo::Type::Air
// 								&& current_block != BlockInfo::Type::Water
// 								&& generateCaveBlock(position) == BlockInfo::Type::Air)
// 								chunk->setBlock(blockX, blockY, blockZ, BlockInfo::Type::Air);
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// 	if (info.level <= LIGHT && info.oldLevel > LIGHT)
// 	{
// 		for(int x = 0; x < info.size.x; x++)
// 		{
// 			for(int z = 0; z < info.size.z; z++)
// 			{
// 				glm::ivec3 chunk_pos_3D = info.start + glm::ivec3(x, 0, z);
// 				std::shared_ptr<Chunk> chunk = chunkGenGrid.at(chunk_pos_3D);
// 				std::lock_guard(chunk->status);
// 				chunk->setGenLevel(LIGHT);

// 				setSkyLight(chunkGenGrid, chunk_pos_3D);
// 				setBlockLight(chunkGenGrid, chunk_pos_3D);
// 			}
// 		}
// 	}
// }

void World::WorldGenerator::setupPass(genStruct & genData, const glm::ivec3 & chunk_pos, Chunk::genLevel gen_level)
{
	std::shared_ptr<Chunk> chunk = m_world.getChunkNoLock(chunk_pos);
	if (chunk == nullptr)
	{
		chunk = std::make_shared<Chunk>(chunk_pos);
		m_world.insertChunkNoLock(chunk_pos, chunk);
	}

	if (chunk->getGenLevel() <= gen_level || genData.generated_chunk.contains({chunk_pos, gen_level}))
		return;
	genData.generated_chunk.insert({chunk_pos, gen_level});

	switch(gen_level)
	{
	case LIGHT:
	{
		if (chunk->getGenLevel() <= LIGHT)
			return;
		//to do lights we need to garantee that the current chunk has full formed relief chunks around it
		for(int x = -1; x <= 1; x++)
		{
			for(int z = -1; z <= 1; z++)
			{
				glm::ivec3 current_pos = chunk_pos + glm::ivec3(x, 0, z);
				setupPass(genData, current_pos, CAVE);
			}
		}
		setupPass(genData, chunk_pos, CAVE);

		genData.light_graph->emplace([this, chunk_pos]{
			lightPass(chunk_pos);
		});
		break;
	}
	case CAVE:
	{
		if (chunk->getGenLevel() <= CAVE)
			return;
		genData.relief_graph->emplace([this, chunk_pos]{
			reliefPass(chunk_pos);
		});
		break;
	}
	default:
		break;
	}
}

std::shared_ptr<task::TaskGraph> World::WorldGenerator::getGenerationGraph(ChunkGenList & chunks_to_gen)
{
	genStruct genData;

	for (auto position : chunks_to_gen)
		setupPass(genData, position, LIGHT);
	return genData.graph;
}

void World::WorldGenerator::lightPass(const glm::ivec3 & chunkPos3D)
{
	ChunkMap chunkGrid;
	std::shared_ptr<Chunk> current_chunk = m_world.getChunkNoLock(chunkPos3D);
	{
		std::lock_guard lock(current_chunk->status);
		current_chunk->setGenLevel(LIGHT);
	}
	for (int x = -1; x <= 1; x++)
	{
		for (int z = -1; z <= 1; z++)
		{
			glm::ivec3 chunk_pos = chunkPos3D + glm::ivec3(x, 0, z);
			std::shared_ptr<Chunk> chunk = m_world.getChunkNoLock(chunk_pos);
			if (chunk == nullptr)
				continue;
			std::lock_guard lock(chunk->status);
			if (chunk->getGenLevel() != CAVE && chunk->getGenLevel() != LIGHT)
				throw std::runtime_error("chunk not generated :" + std::to_string(chunk_pos.x) + " " + std::to_string(chunk_pos.z));
			chunkGrid.insert({chunk_pos, chunk});
		}
	}

	std::scoped_lock lock(
		chunkGrid.at(chunkPos3D)->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{0, 0, -1})->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{0, 0, 1})->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{1, 0, -1})->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{1, 0, 0})->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{1, 0, 1})->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{-1, 0, -1})->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{-1, 0, 0})->status,
		chunkGrid.at(chunkPos3D + glm::ivec3{-1, 0, 1})->status
	);

	setSkyLight(chunkGrid, chunkPos3D);
	setBlockLight(chunkGrid, chunkPos3D);
}

void World::WorldGenerator::reliefPass(const glm::ivec3 & chunkPos3D)
{
	std::shared_ptr<Chunk> chunk = m_world.getChunkNoLock(chunkPos3D);
	std::lock_guard(chunk->status);
	chunk->setGenLevel(CAVE);
	for(int blockX = 0; blockX < CHUNK_X_SIZE; blockX++)
	{
		for(int blockZ = 0; blockZ < CHUNK_Z_SIZE; blockZ++)
		{
			// check the continentalness of the chunk
			float continentalness = m_continentalness_perlin.noise(glm::vec2(
				(blockX + chunkPos3D.x * CHUNK_X_SIZE) * 0.00090f,
				(blockZ + chunkPos3D.z * CHUNK_Z_SIZE) * 0.00090f
			));

			// continentalness *= 4; // map from [-1, 1] to [-4, 4]

			//generate the relief value for the whole chunk
			float reliefValue = generateReliefValue(glm::ivec2(
				blockX + chunkPos3D.x * CHUNK_X_SIZE,
				blockZ + chunkPos3D.z * CHUNK_Z_SIZE
			));

			float riverValue = std::abs(reliefValue);

			reliefValue = (reliefValue + 1) / 2; // map from [-1, 1] to [0, 1]
			reliefValue = pow(2, 10 * reliefValue - 10); // map from [0, 1] to [0, 1] with a slope

			float oceanReliefValue = (reliefValue * -60) + 60; // map from [0, 1] to [60, 0]
			float landReliefValue = (reliefValue * (CHUNK_Y_SIZE - 80)) + 80; // map from [0, 1] to [80, CHUNK_Y_SIZE]

			bool isLand = continentalness > 0.2f;
			bool isOcean = continentalness < 0.00f;
			bool isCoast = !isLand && !isOcean;

			if (isOcean)
			{
				reliefValue = oceanReliefValue;
			}
			else if (isCoast)
			{
				// reliefValue *= (CHUNK_Y_SIZE - 100); // map from [0, 1] to [0, CHUNK_Y_SIZE - 100]
				// reliefValue += 100; // map from [0, CHUNK_Y_SIZE - 100] to [100, CHUNK_Y_SIZE]
				reliefValue = std::lerp(oceanReliefValue, landReliefValue, mapRange(continentalness, 0.0f, 0.2f, 0.0f, 1.0f));
			}
			else if (isLand)
			{
				// reliefValue *= (CHUNK_Y_SIZE - 100); // map from [0, 1] to [0, CHUNK_Y_SIZE - 100]
				// reliefValue += 100; // map from [0, CHUNK_Y_SIZE - 100] to [100, CHUNK_Y_SIZE]
				reliefValue = landReliefValue;
			}

			for(int blockY = 0; blockY < CHUNK_Y_SIZE; blockY++)
			{

				glm::ivec3 position = glm::ivec3(
					blockX + chunkPos3D.x * CHUNK_X_SIZE,
					blockY,
					blockZ + chunkPos3D.z * CHUNK_Z_SIZE
				);
				BlockInfo::Type to_set = BlockInfo::Type::Air;

				if (isOcean)
				{
					if (position.y < reliefValue)
						to_set = BlockInfo::Type::Stone;
					else if (position.y < 80)
						to_set = BlockInfo::Type::Water;
					else
						to_set = BlockInfo::Type::Air;
				}
				else if (isCoast)
				{
					if (position.y > 80)
					{
						if (position.y < reliefValue - 5)
							to_set = BlockInfo::Type::Stone;
						else if (position.y < reliefValue - 1 )
							to_set = BlockInfo::Type::Dirt;
						else if (position.y < reliefValue)
							to_set = BlockInfo::Type::Grass;
						else
							to_set = BlockInfo::Type::Air;
					}
					else
					{
						if (position.y < reliefValue)
							to_set = BlockInfo::Type::Stone;
						else
							to_set = BlockInfo::Type::Water;
					}
				}
				else if (isLand)
				{
					(void)riverValue;
					//check to see wether above or below the relief value
					// if (reliefValue > position.y)
						// to_set = BlockInfo::Type::Stone;
					// else if (reliefValue + 5 > position.y)
					// {
					// 	if (riverValue < 0.05f)
					// 		to_set = BlockInfo::Type::Water;
					// 	else
					// 		to_set = BlockInfo::Type::Grass;
					// }
					// else
						// to_set = BlockInfo::Type::Air;
					if (position.y < reliefValue - 5)
						to_set = BlockInfo::Type::Stone;
					else if (position.y < reliefValue - 1)
						to_set = BlockInfo::Type::Dirt;
					else if (position.y < reliefValue)
						to_set = BlockInfo::Type::Grass;
					else
						to_set = BlockInfo::Type::Air;
				}

				if (to_set != BlockInfo::Type::Air
					&& to_set != BlockInfo::Type::Water
					&& generateCaveBlock(position) == BlockInfo::Type::Air)
					to_set = BlockInfo::Type::Air;
				chunk->setBlock(blockX, blockY, blockZ, to_set);
			}
		}
	}
}
