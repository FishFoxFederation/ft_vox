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
	for (int x = 0; x < 10; x++)
	{
		for (int z = 0; z < 10; z++)
		{
			for (int y = 0; y < 10; y++)
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

	for (auto & [position, chunk] : m_world.chunks())
	{
		uint64_t mesh_id = m_vulkanAPI.createMesh(chunk);
		if (mesh_id != VulkanAPI::no_mesh_id)
		{
			m_worldScene.addMeshData(mesh_id, glm::vec3(position * CHUNK_SIZE));
		}
	}
}

void BlockUpdateThread::loop()
{
}
