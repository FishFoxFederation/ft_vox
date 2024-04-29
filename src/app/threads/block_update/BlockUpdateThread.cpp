#include "BlockUpdateThread.hpp"

#include <unistd.h>

BlockUpdateThread::BlockUpdateThread(
	WorldScene & worldScene,
	World & world
):
	m_worldScene(worldScene),
	m_world(world)
{
}

BlockUpdateThread::~BlockUpdateThread()
{
	this->m_thread.request_stop();
	this->m_thread.join();
}

void BlockUpdateThread::init()
{
	LOG_INFO("BlockUpdateThread launched :" << gettid());
	// for (int x = 0; x < 10; x++)
	// {
	// 	for (int z = 0; z < 10; z++)
	// 	{
	// 		for (int y = 0; y < 16; y++)
	// 		{
	// 			m_world.chunks().insert(
	// 				std::make_pair(
	// 					glm::vec3(x, y, z),
	// 					m_world.m_worldGenerator.generateChunk(x, y, z)
	// 				)
	// 			);
	// 		}
	// 	}
	// }
	// LOG_DEBUG("AVG perlin :" << m_world.m_worldGenerator.m_avg / m_world.m_worldGenerator.m_called);
	// LOG_DEBUG("MAX perlin :" << m_world.m_worldGenerator.m_max);
	// LOG_DEBUG("MIN perlin :" << m_world.m_worldGenerator.m_min);

	// for (auto & [pos, chunk] : m_world.chunks())
	// {
	// 	uint64_t mesh_id = m_vulkanAPI.createMesh(
	// 		chunk,
	// 		pos.x < 9 ? &m_world.chunks().at(glm::vec3(pos.x + 1, pos.y, pos.z)) : nullptr,
	// 		pos.x > 0 ? &m_world.chunks().at(glm::vec3(pos.x - 1, pos.y, pos.z)) : nullptr,
	// 		pos.y < 15 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y + 1, pos.z)) : nullptr,
	// 		pos.y > 0 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y - 1, pos.z)) : nullptr,
	// 		pos.z < 9 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y, pos.z + 1)) : nullptr,
	// 		pos.z > 0 ? &m_world.chunks().at(glm::vec3(pos.x, pos.y, pos.z - 1)) : nullptr
	// 	);
	// 	if (mesh_id != VulkanAPI::no_mesh_id)
	// 	{
	// 		m_worldScene.addMeshData(mesh_id, glm::vec3(pos * CHUNK_SIZE));
	// 	}
	// }
}

void BlockUpdateThread::loop()
{
	m_world.updateBlock(m_worldScene.camera().getPosition());
	// LOG_INFO("PLAYER POSITION: " << m_worldScene.camera().position().x << " " << m_worldScene.camera().position().y << " " << m_worldScene.camera().position().z);
}
