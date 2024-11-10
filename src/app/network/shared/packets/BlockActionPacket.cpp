#include "BlockActionPacket.hpp"

BlockActionPacket::BlockActionPacket()
{
}

BlockActionPacket::BlockActionPacket(BlockInfo::Type block_id, glm::vec3 position, Action action)
: m_block_id(block_id), m_position(position), m_action(action)
{
}

BlockActionPacket::BlockActionPacket(const BlockActionPacket & other)
: IPacket(other), m_block_id(other.m_block_id), m_position(other.m_position), m_action(other.m_action)
{
}

BlockActionPacket & BlockActionPacket::operator=(const BlockActionPacket & other)
{
	if (this != &other)
	{
		m_block_id = other.m_block_id;
		m_position = other.m_position;
		m_action = other.m_action;
		::IPacket::operator=(other);
	}
	return *this;
}

BlockActionPacket::BlockActionPacket(BlockActionPacket && other)
: IPacket(other), m_block_id(other.m_block_id), m_position(other.m_position), m_action(other.m_action)
{
}

BlockActionPacket & BlockActionPacket::operator=(BlockActionPacket && other)
{
	if (this != &other)
	{
		m_block_id = other.m_block_id;
		m_position = other.m_position;
		m_action = other.m_action;
		::IPacket::operator=(other);
	}
	return *this;
}

BlockActionPacket::~BlockActionPacket()
{
}

void BlockActionPacket::Serialize(uint8_t * buffer) const
{
	//HEADER
	buffer += SerializeHeader(buffer);

	// BODY
	memcpy(buffer, &m_block_id, sizeof(m_block_id));
	buffer += sizeof(m_block_id);

	memcpy(buffer, &m_position, sizeof(m_position));
	buffer += sizeof(m_position);

	memcpy(buffer, &m_action, sizeof(m_action));
	buffer += sizeof(m_action);
}

void BlockActionPacket::Deserialize(const uint8_t * buffer)
{
	//skip over the header
	buffer += sizeof(IPacket::STATIC_HEADER_SIZE);

	memcpy(&m_block_id, buffer, sizeof(m_block_id));
	buffer += sizeof(m_block_id);

	memcpy(&m_position, buffer, sizeof(m_position));
	buffer += sizeof(m_position);

	memcpy(&m_action, buffer, sizeof(m_action));
	buffer += sizeof(m_action);
}

uint32_t BlockActionPacket::Size() const
{
	return IPacket::STATIC_HEADER_SIZE + sizeof(m_block_id) + sizeof(m_position) + sizeof(m_action);
}

bool BlockActionPacket::HasDynamicSize() const
{
	return false;
}

IPacket::Type BlockActionPacket::GetType() const
{
	return IPacket::Type::BLOCK_ACTION;
}

std::shared_ptr<IPacket> BlockActionPacket::Clone() const
{
	return std::make_shared<BlockActionPacket>();
}

BlockInfo::Type BlockActionPacket::GetBlockID() const
{
	return m_block_id;
}

glm::vec3 BlockActionPacket::GetPosition() const
{
	return m_position;
}

BlockActionPacket::Action BlockActionPacket::GetAction() const
{
	return m_action;
}

void BlockActionPacket::SetBlockID(BlockInfo::Type block_id)
{
	m_block_id = block_id;
}

void BlockActionPacket::SetPosition(glm::vec3 position)
{
	m_position = position;
}

void BlockActionPacket::SetAction(Action action)
{
	m_action = action;
}
