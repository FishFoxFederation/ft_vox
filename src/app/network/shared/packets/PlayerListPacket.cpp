#include "PlayerListPacket.hpp"

PlayerListPacket::PlayerListPacket()
{
}

PlayerListPacket::~PlayerListPacket()
{
}

PlayerListPacket::PlayerListPacket(const std::vector<PlayerInfo>& players)
	: m_players(players)
{
}

PlayerListPacket::PlayerListPacket(const PlayerListPacket & other)
	: IPacket(other), m_players(other.m_players)
{
}

PlayerListPacket::PlayerListPacket(PlayerListPacket && other)
	: IPacket(std::move(other)), m_players(std::move(other.m_players))
{
}

PlayerListPacket & PlayerListPacket::operator=(const PlayerListPacket & other)
{
	if (this != &other)
	{
		m_players = other.m_players;
		::IPacket::operator=(other);
	}
	return *this;
}

PlayerListPacket & PlayerListPacket::operator=(PlayerListPacket && other)
{
	if (this != &other)
	{
		m_players = std::move(other.m_players);
		::IPacket::operator=(std::move(other));
	}
	return *this;
}

void PlayerListPacket::Serialize(uint8_t * buffer) const
{
	buffer += SerializeHeader(buffer);

	for (const auto & player : m_players)
	{
		std::memcpy(buffer, &player.id, sizeof(player.id));
		buffer += sizeof(player.id);

		std::memcpy(buffer, &player.position, sizeof(player.position));
		buffer += sizeof(player.position);
	}
}

void PlayerListPacket::Deserialize(const uint8_t * buffer)
{
	//skip over the type
	buffer += sizeof(IPacket::Type);

	//extract the size from the header
	size_t size;
	std::memcpy(&size, buffer, sizeof(size));
	buffer += sizeof(size);

	size -= IPacket::DYNAMIC_HEADER_SIZE;
	size /= (sizeof(PlayerInfo::id) + sizeof(PlayerInfo::position));
	m_players.clear();
	m_players.reserve(size);
	for (size_t i = 0; i < size; ++i)
	{
		PlayerInfo player;
		std::memcpy(&player.id, buffer, sizeof(player.id));
		buffer += sizeof(player.id);

		std::memcpy(&player.position, buffer, sizeof(player.position));
		buffer += sizeof(player.position);

		m_players.push_back(player);
	}
}

uint32_t PlayerListPacket::Size() const
{
	return IPacket::DYNAMIC_HEADER_SIZE + m_players.size() * (sizeof(PlayerInfo::id) + sizeof(PlayerInfo::position));
}

bool PlayerListPacket::HasDynamicSize() const
{
	return true;
}

IPacket::Type PlayerListPacket::GetType() const
{
	return IPacket::Type::PLAYER_LIST;
}

std::shared_ptr<IPacket> PlayerListPacket::Clone() const
{
	return std::make_shared<PlayerListPacket>();
}

const std::vector<PlayerListPacket::PlayerInfo>& PlayerListPacket::GetPlayers() const
{
	return m_players;
}

void PlayerListPacket::SetPlayers(const std::vector<PlayerListPacket::PlayerInfo>& players)
{
	m_players = players;
}
