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
	hitbox(hitbox),
	velocity({0.0, 0.0, 0.0})
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
	Entity(HitBox({-0.4, 0, -0.4}, {0.8, 1.8, 0.8})),
	gameMode(GameMode::SPECTATOR),
	feet({-0.4, -0.1, -0.4}, {0.8, 0.1, 0.8}),
	m_yaw(0.0),
	m_pitch(0.0)
{
}

Player::~Player()
{
}

void Player::moveDirection(double x_offset, double y_offset)
{
	m_yaw += x_offset;
	m_pitch = glm::clamp(m_pitch - y_offset, -89.0, 89.0);

	// updateTransform();
}

glm::dvec3 Player::direction() const
{
	return glm::normalize(glm::dvec3(
		cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw)),
		sin(glm::radians(m_pitch)),
		cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw))
	));
}

glm::dvec3 Player::getDisplacement(glm::dvec3 move) const
{
	static const glm::dvec3 up_vec = glm::dvec3(0.0, 1.0, 0.0);
	const glm::dvec3 dir_vec = direction();
	glm::dvec3 displacement = move.x * glm::normalize(glm::cross(dir_vec, up_vec))
							+ move.y * up_vec
							+ move.z * glm::normalize(glm::dvec3(dir_vec.x, 0.0, dir_vec.z));
	return displacement;
}

glm::dvec3 Player::eyePosition() const
{
	double eye_height = 1.6;
	// First person view
	// return glm::dvec3(0.0, eye_height, 0.0);

	// Third person view
	const double distance = 5;
	const glm::dvec3 dir_vec = direction();
	return glm::dvec3(0.0, eye_height, 0.0) - distance * dir_vec;
}

void Player::updateTransform()
{
	transform.rotation.y = -glm::radians(m_yaw);
}

bool Player::shouldCollide() const
{
	return gameMode != GameMode::SPECTATOR;
}

bool Player::shouldFall() const
{
	return gameMode == GameMode::SURVIVAL || (gameMode == GameMode::CREATIVE && !flying);
}

void Player::startFall()
{
	fall_start_time = std::chrono::steady_clock::now().time_since_epoch();
	fall_start_position = transform.position;
}

double Player::fallDuration()
{
	return static_cast<double>((std::chrono::steady_clock::now().time_since_epoch() - fall_start_time).count()) / 1e9;
}
