#include "Player.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

//############################################################################################################
//                                                                                                           #
//                                                  Player                                                   #
//                                                                                                           #
//############################################################################################################

Player::Player()
{
}

Player::~Player()
{
}

void Player::moveDirection(double x_offset, double y_offset)
{
	yaw += x_offset;
	pitch = glm::clamp(pitch - y_offset, -89.0, 89.0);
}

Camera Player::camera() const
{
	return Camera(transform.position + eyePosition(), pitch, yaw);
}

glm::dvec3 Player::direction() const
{
	return glm::normalize(glm::dvec3(
		cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
		sin(glm::radians(pitch)),
		cos(glm::radians(pitch)) * sin(glm::radians(yaw))
	));
}

glm::dvec3 Player::getTransformedMovement(glm::dvec3 move) const
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
	return glm::dvec3(0.0, eye_height, 0.0);

	// Third person view
	// const double distance = 5;
	// const glm::dvec3 dir_vec = direction();
	// return glm::dvec3(0.0, eye_height, 0.0) - distance * dir_vec;
}

void Player::updateTransform()
{
	transform.rotation.y = -glm::radians(yaw);
}

bool Player::shouldCollide() const
{
	return gameMode != GameMode::SPECTATOR;
}

bool Player::shouldFall() const
{
	return gameMode == GameMode::SURVIVAL || (gameMode == GameMode::CREATIVE && !flying);
}

bool Player::isFlying() const
{
	return gameMode == GameMode::SPECTATOR || (gameMode == GameMode::CREATIVE && flying);
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

bool Player::canJump() const
{
	return (on_ground || std::chrono::steady_clock::now().time_since_epoch() - fall_start_time < jump_delai_after_fall)
		&& jump_remaining > 0
		&& std::chrono::steady_clock::now().time_since_epoch() - last_jump_time > jump_delai;
}

void Player::startJump()
{
	jump_remaining--;
	jumping = true;
	last_jump_time = std::chrono::steady_clock::now().time_since_epoch();
}

bool Player::canAttack() const
{
	return std::chrono::steady_clock::now().time_since_epoch() - last_attack_time > attack_delai;
}

void Player::startAttack()
{
	last_attack_time = std::chrono::steady_clock::now().time_since_epoch();
}

bool Player::canUse() const
{
	return std::chrono::steady_clock::now().time_since_epoch() - last_use_time > use_delai;
}

void Player::startUse()
{
	last_use_time = std::chrono::steady_clock::now().time_since_epoch();
}
