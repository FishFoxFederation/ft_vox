#include "BlockUpdateThread.hpp"

BlockUpdateThread::BlockUpdateThread(
	WorldScene & worldScene,
	VulkanAPI & vulkanAPI
):
	m_worldScene(worldScene),
	m_vulkanAPI(vulkanAPI
)
{
}

BlockUpdateThread::~BlockUpdateThread()
{
}

void BlockUpdateThread::init()
{
	int size_x = 16;
	int size_y = 16;
	int size_z = 16;
	for (int x = 0; x < size_x; x++)
	{
		for (int z = 0; z < size_z; z++)
		{
			for (int y = 0; y < size_y; y++)
			{
				m_world.chunks().insert(
					std::make_pair(
						glm::vec3(x, y, z),
						m_world.m_worldGenerator.generateChunk(x, y, z)
					)
				);
			}
		}
	}
	LOG_DEBUG("AVG perlin :" << m_world.m_worldGenerator.m_avg / m_world.m_worldGenerator.m_called);
	LOG_DEBUG("MAX perlin :" << m_world.m_worldGenerator.m_max);
	LOG_DEBUG("MIN perlin :" << m_world.m_worldGenerator.m_min);

	for (auto & [pos, chunk] : m_world.chunks())
	{
		uint64_t mesh_id = m_vulkanAPI.createMesh(
			chunk,
			pos.x < size_x - 1 ? &m_world.chunks().at(glm::vec3(pos.x + 1, pos.y, pos.z)) : nullptr,
			pos.x > 0 ? &m_world.chunks().at(glm::vec3(pos.x - 1, pos.y, pos.z)) : nullptr,
			pos.y < size_y - 1 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y + 1, pos.z)) : nullptr,
			pos.y > 0 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y - 1, pos.z)) : nullptr,
			pos.z < size_z - 1 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y, pos.z + 1)) : nullptr,
			pos.z > 0 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y, pos.z - 1)) : nullptr
		);
		if (mesh_id != VulkanAPI::no_mesh_id)
		{
			m_worldScene.addMeshData(mesh_id, glm::vec3(pos * CHUNK_SIZE));
		}
	}
}

void BlockUpdateThread::loop()
{
}
