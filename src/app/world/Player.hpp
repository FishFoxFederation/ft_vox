#pragma once

#include "define.hpp"
#include <glm/vec3.hpp>
#include <mutex>

class Player 
{
public:
	Player();
	Player(const glm::vec3 & position, const glm::vec3 & rotation);
	~Player();

	glm::vec3	position() const;
	glm::vec3	rotation() const;

	void		setPosition(const glm::vec3 & position);
	void		setRotation(const glm::vec3 & rotation);

	void		move(const glm::vec3 & movement);
private:
	glm::vec3 m_position;
	glm::vec3 m_rotation;

	mutable std::mutex m_mutex;
};
