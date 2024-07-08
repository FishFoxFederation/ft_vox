#include "TicketManager.hpp"

TicketManager::TicketManager(
	std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> & chunks,
	std::mutex & chunks_mutex
)
: m_chunks(chunks), m_chunks_mutex(chunks_mutex)
{
}

TicketManager::~TicketManager()
{
}

std::vector<glm::ivec3> TicketManager::getBlockUpdateChunks() const
{
	std::vector<glm::ivec3> chunk_positions;
	std::lock_guard lock(m_chunks_mutex);
	for (auto & [position, chunk] : m_chunks)
	{
		if (chunk->getLoadLevel() <= TICKET_LEVEL_BLOCK_UPDATE)
			chunk_positions.push_back(position);
	}
	return chunk_positions;
}

std::vector<glm::ivec3> TicketManager::getEntityUpdateChunks() const
{
	std::vector<glm::ivec3> chunk_positions;
	std::lock_guard lock(m_chunks_mutex);
	for (auto & [position, chunk] : m_chunks)
	{
		if (chunk->getLoadLevel() <= TICKET_LEVEL_ENTITY_UPDATE)
			chunk_positions.push_back(position);
	}
	return chunk_positions;
}

std::vector<TicketManager::Ticket> TicketManager::getTickets() const
{
	std::lock_guard lock(m_mutex);
	return m_active_tickets;
}

void TicketManager::addTicket(const TicketManager::Ticket & ticket)
{
	std::lock_guard lock(m_mutex);
	m_tickets_to_add.push_back(ticket);
}

void TicketManager::removeTicket(const TicketManager::Ticket & ticket)
{
	std::lock_guard lock(m_mutex);
	m_tickets_to_remove.push_back(ticket);
}

void TicketManager::updateTickets()
{
	std::lock_guard lock(m_mutex);
	std::lock_guard lock2(m_chunks_mutex);

	if (!m_tickets_to_remove.empty())
	{
		for (auto & ticket : m_tickets_to_remove)
		{
			auto it = std::find(m_active_tickets.begin(), m_active_tickets.end(), ticket);
			if (it != m_active_tickets.end())
			{
				m_active_tickets.erase(it);
			}
		}
		m_tickets_to_remove.clear();
		clearChunksLevels();
	}

	if (!m_tickets_to_add.empty())
	{
		for (auto & ticket : m_tickets_to_add)
			m_active_tickets.push_back(ticket);
		m_tickets_to_add.clear();
	}

	floodFill(m_active_tickets);
}

void TicketManager::floodFill(const std::vector<TicketManager::Ticket> & tickets)
{
	std::queue<std::pair<glm::ivec3, int>> queue;
	for (auto & ticket : tickets)
	{
		queue.push(std::make_pair(ticket.position, ticket.level));
	}
	while (!queue.empty())
	{
		auto [current_position, current_level] = queue.front();
		queue.pop();
		std::shared_ptr<Chunk> chunk = m_chunks[current_position];
		if (current_level > 44)
			continue;
		if (current_level < chunk->getLoadLevel())
		{
			chunk->setLoadLevel(current_level);
			for (int x = -1; x <= 1; x++)
			{
				for (int y = -1; y <= 1; y++)
				{
					for (int z = -1; z <= 1; z++)
					{
						glm::ivec3 new_position = current_position + glm::ivec3(x, y, z);
						std::shared_ptr<Chunk> current_chunk = m_world.getChunk(new_position);
						if (current_chunk == nullptr)
							current_chunk = std::make_shared<Chunk>(); 
						{
							queue.push(std::make_pair(new_position, current_level + 1));
						}
					}
				}
			}
		}
	}
}

void TicketManager::clearChunksLevels()
{
	for (auto & [position, chunk] : m_chunks)
		chunk->setLoadLevel(TICKET_LEVEL_INACTIVE);
}
