#include "ServerWorld.hpp"

ServerWorld::ServerWorld(Server & server)
:	World(true),
	m_server(server),
	m_executor(task::Executor::getInstance())
{
	addTicket({Ticket::Type::OTHER, SPAWN_TICKET_LEVEL, glm::ivec3(0, 0, 0)});
	// std::shared_ptr<Chunk> chunk;

	// std::lock_guard lock(m_chunks_mutex);

	// chunk = m_world_generator.generateChunkColumn(0, 0);
	// this->m_chunks.insert({glm::ivec3(0, 0, 0), chunk});

	// chunk = m_world_generator.generateChunkColumn(0, 1);
	// this->m_chunks.insert({glm::ivec3(0, 0, 1), chunk});

	// chunk = m_world_generator.generateChunkColumn(1, 0);
	// this->m_chunks.insert({glm::ivec3(1, 0, 0), chunk});

	// chunk = m_world_generator.generateChunkColumn(1, 1);
	// this->m_chunks.insert({glm::ivec3(1, 0, 1), chunk});
}

ServerWorld::~ServerWorld()
{
}

void ServerWorld::update()
{
	ZoneScopedN("Block Update");
	std::lock_guard lock(m_players_info_mutex);
	{
		// std::lock_guard lock(m_tickets_mutex);

		// LOG_INFO("BU size: " << m_block_update_chunks.size());
	}

	//update all values that need to be updated
	updateTickedValues();

	savePlayerPositions();
	// MAIN BLOCK UPDATE FUNCTION

	// do all block updates
	updateBlocks();

	updateLights();

	// do all chunk updates
	updateTickets();

	// if player changed chunk send new chunks and update observations
	updatePlayerPositions();
}

void ServerWorld::savePlayerPositions()
{
	std::lock_guard lock(m_players_mutex);

	m_current_tick_player_positions.clear();
	for (auto & [player_id, player] : m_players)
	{
		std::lock_guard lock(player->mutex);
		m_current_tick_player_positions.insert({player_id, player->transform.position});
		if (m_last_tick_player_positions.contains(player_id))
		{
			glm::ivec3 last_chunk = getChunkPosition(m_last_tick_player_positions.at(player_id));
			last_chunk.y = 0;
			glm::ivec3 current_chunk = getChunkPosition(player->transform.position);
			current_chunk.y = 0;

			//change ticket if player changed chunk
			if (last_chunk != current_chunk)
			{
				Ticket next_ticket{ Ticket::Type::PLAYER, m_player_ticket_level, current_chunk };
				player->player_ticket_id = changeTicket(player->player_ticket_id, next_ticket);
			}
		}
	}
}

void ServerWorld::updatePlayerPositions()
{
	ZoneScoped;
	static int old_load_distance = getLoadDistance();

	if (old_load_distance != getLoadDistance())
		LOG_INFO("Load distance changed");

	std::lock_guard lock(m_players_mutex);
	for (auto & [player_id, current_pos] : m_current_tick_player_positions)
	{
		if (old_load_distance != getLoadDistance())
			LOG_INFO("IN LOOP Load distance changed");
		ChunkLoadUnloadData data = updateChunkObservations(player_id, old_load_distance);
		sendChunkLoadUnloadData(data, player_id);
		m_last_tick_player_positions.at(player_id) = m_current_tick_player_positions.at(player_id);
	}
	old_load_distance = getLoadDistance();
}

void ServerWorld::waitForChunkFutures()
{
	ZoneScopedN("Wait For Chunk Futures");

	std::lock_guard lock(m_chunk_gen_data.m_chunk_gen_data_mutex);
	m_chunk_gen_data.future.get();
}

void ServerWorld::updateTickedValues()
{
	auto & updates = m_ticked_updates;
	updates.player_ticket_level_changed =
		updates.player_ticket_level != m_player_ticket_level;
	m_player_ticket_level = m_ticked_updates.player_ticket_level;
}

int ServerWorld::getLoadDistance() const
{
	return TICKET_LEVEL_INACTIVE - m_player_ticket_level;
}
