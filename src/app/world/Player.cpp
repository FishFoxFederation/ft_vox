#include "Player.hpp"

Player::Player()
	: m_position(0.0f, 0.0f, 0.0f)
	, m_rotation(0.0f, 0.0f, 0.0f)
{
}

Player::Player(const glm::vec3 & position, const glm::vec3 & rotation)
	: m_position(position)
	, m_rotation(rotation)
{
}

Player::~Player()
{

}

glm::vec3 Player::position() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_position;
}

glm::vec3 Player::rotation() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_rotation;
}

void Player::setPosition(const glm::vec3 & position)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_position = position;
}

void Player::setRotation(const glm::vec3 & rotation)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_rotation = rotation;
}
	
void Player::move(const glm::vec3 & movement)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_position += movement;
}
