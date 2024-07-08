#include "ServerWorld.hpp"

std::vector<glm::ivec3> ServerWorld::getBlockUpdateChunks() const
{
	std::vector<glm::ivec3> chunks;
	std::lock_guard lock(m_chunks_mutex);
	for (auto & [position, chunk] : m_chunks)
	{
		if (chunk->getLoadLevel() == TICKET_LEVEL_BLOCK_UPDATE)
			chunks.push_back(position);
	}
	return chunks;
}

std::vector<glm::ivec3> ServerWorld::getEntityUpdateChunks() const
{
	std::vector<glm::ivec3> chunks;
	std::lock_guard lock(m_chunks_mutex);
	for (auto & [position, chunk] : m_chunks)
	{
		if (chunk->getLoadLevel() == TICKET_LEVEL_ENTITY_UPDATE)
			chunks.push_back(position);
	}
	return chunks;
}

std::vector<ServerWorld::Ticket> ServerWorld::getTickets() const
{
	std::lock_guard lock(m_tickets_mutex);
	return m_active_tickets;
}

void ServerWorld::addTicket(const ServerWorld::Ticket & ticket)
{
	std::lock_guard lock(m_tickets_mutex);
	m_tickets_to_add.push_back(ticket);
}

void ServerWorld::removeTicket(const ServerWorld::Ticket & ticket)
{
	std::lock_guard lock(m_tickets_mutex);
	m_tickets_to_remove.push_back(ticket);
}

void ServerWorld::updateTickets()
{
	std::lock_guard lock(m_tickets_mutex);
	std::lock_guard lock2(m_chunks_mutex);
	bool changed = false;

	if (!m_tickets_to_remove.empty())
	{
		changed = true;
		for (auto & ticket : m_tickets_to_remove)
		{
			auto it = std::find(m_active_tickets.begin(), m_active_tickets.end(), ticket);
			if (it != m_active_tickets.end())
				m_active_tickets.erase(it);
		}
		clearChunksLoadLevel();
	}

	if (!m_tickets_to_add.empty())
	{
		changed = true;
		for (auto & ticket : m_tickets_to_add)
		{
			m_active_tickets.push_back(ticket);
		}
		//no need to clear when only adding to a floodfill
	}

	if (changed)
	{
		floodFill(m_active_tickets);
	}
}

void ServerWorld::clearChunksLoadLevel()
{
	for (auto & [position, chunk] : m_chunks)
		chunk->setLoadLevel(TICKET_LEVEL_INACTIVE);
	m_block_update_chunks.clear();
	m_entity_update_chunks.clear();
	m_border_chunks.clear();
}

void ServerWorld::floodFill(const std::vector<ServerWorld::Ticket> & tickets)
{
	std::queue<Ticket> queue;
	for (auto & ticket : tickets)
		queue.push(ticket);

	while (!queue.empty())
	{
		auto current = queue.front();
		queue.pop();
		if (current.level > 44)
			continue;

		//visit the chunk

		std::shared_ptr<Chunk> chunk = getChunk(current.position);
		
		if (chunk == nullptr)
			chunk = std::make_shared<Chunk>(current.position);
		if (current.level >= chunk->getLoadLevel())
			continue;
		if (current.level <= TICKET_LEVEL_INACTIVE && !chunk->isGenerated())
			m_world_generator.generateChunkColumn(current.position.x, current.position.z, chunk);
		
		if (current.level <= TICKET_LEVEL_ENTITY_UPDATE)
			m_entity_update_chunks.insert(current.position);
		if (current.level <= TICKET_LEVEL_BLOCK_UPDATE)
			m_block_update_chunks.insert(current.position);
		if (current.level <= TICKET_LEVEL_BORDER)
			m_border_chunks.insert(current.position);

		chunk->setLoadLevel(current.level);


		// add the neighbors
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					glm::ivec3 new_position = current.position + glm::ivec3(x, y, z);

					queue.push(Ticket{current.level + 1, new_position});
				}
			}
		}
	}
}
