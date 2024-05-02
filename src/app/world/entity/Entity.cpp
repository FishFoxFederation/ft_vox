#include "Entity.hpp"
#include "logger.hpp"

//############################################################################################################
//                                                                                                           #
//                                                  Entity                                                   #
//                                                                                                           #
//############################################################################################################

Entity::Entity(
	const HitBox & hitbox
):
	hitbox(hitbox)
{
}

Entity::~Entity()
{
}


//############################################################################################################
//                                                                                                           #
//                                                  Player                                                   #
//                                                                                                           #
//############################################################################################################

Player::Player():
	Entity(HitBox({-0.5, 0, -0.5}, {1, 1, 1})),
	m_eye_position(0.0, 1.6, 0.0),
	m_yaw(0.0),
	m_pitch(0.0)
{
}

Player::~Player()
{
}

void Player::movePosition(glm::dvec3 displacement)
{
	transform.position += displacement;
}

void Player::moveDirection(double x_offset, double y_offset)
{
	m_yaw += x_offset;
	m_pitch = glm::clamp(m_pitch - y_offset, -89.0, 89.0);
}

glm::dvec3 Player::direction() const
{
	return glm::dvec3(
		cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw)),
		sin(glm::radians(m_pitch)),
		cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw))
	);
}

glm::dvec3 Player::getDisplacement(glm::dvec3 move) const
{
	static const glm::dvec3 up_vec = glm::dvec3(0.0, 1.0, 0.0);
	glm::dvec3 dir_vec = direction();
	glm::dvec3 displacement = move.x * glm::normalize(glm::cross(dir_vec, up_vec))
							+ move.y * up_vec
							+ move.z * glm::normalize(glm::dvec3(dir_vec.x, 0.0, dir_vec.z));
	return displacement;
}
