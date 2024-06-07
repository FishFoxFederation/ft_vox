#include "WorldScene.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

#include <algorithm>

WorldScene::WorldScene()
{
}

WorldScene::~WorldScene()
{
}

void WorldScene::setTargetBlock(const std::optional<glm::vec3> & target_block)
{
	std::lock_guard<std::mutex> lock(m_target_block_mutex);
	m_target_block = target_block;
}

std::optional<glm::vec3> WorldScene::targetBlock() const
{
	std::lock_guard<std::mutex> lock(m_target_block_mutex);
	return m_target_block;
}

void WorldScene::setDebugBlock(const std::vector<DebugBlock> & debug_block)
{
	std::lock_guard<std::mutex> lock(m_debug_block_mutex);
	m_debug_block = debug_block;
}

void WorldScene::clearDebugBlocks()
{
	std::lock_guard<std::mutex> lock(m_debug_block_mutex);
	m_debug_block.clear();
}

std::vector<WorldScene::DebugBlock> WorldScene::debugBlocks() const
{
	std::lock_guard<std::mutex> lock(m_debug_block_mutex);
	return m_debug_block;
}

std::vector<WorldScene::PlayerRenderData> WorldScene::getPlayers() const
{
	std::lock_guard<std::mutex> lock(m_player_mutex);
	std::vector<PlayerRenderData> players;
	std::transform(
		m_players.begin(),
		m_players.end(),
		std::back_inserter(players),
		[](const auto & player)
		{
			return PlayerRenderData{player.second};
		}
	);
	return players;
}
