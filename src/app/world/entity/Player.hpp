#pragma once

#include "define.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "HitBox.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>

class Player
{

public:

	Player();
	~Player();

	Player(Player & other) = delete;
	Player(Player && other) = delete;
	Player & operator=(Player & other) = delete;
	Player & operator=(Player && other) = delete;

	void moveDirection(double x_offset, double y_offset);

	Camera camera() const
	{
		return Camera(transform.position + eyePosition(), pitch, yaw);
	}

	glm::dvec3 getTransformedMovement(glm::dvec3 move) const;

	glm::dvec3 eyePosition() const; // the position of the eyes relative to entity position
	glm::dvec3 direction() const;

	bool shouldCollide() const;
	bool shouldFall() const;

	void startFall();
	double fallDuration();

	std::mutex mutex;

	enum class GameMode
	{
		SURVIVAL,
		CREATIVE,
		SPECTATOR
	} gameMode = GameMode::SPECTATOR;

	double default_speed = 4.0;
	double sprint_speed_factor = 1.5;
	double sneak_speed_factor = 0.3;
	double fly_speed_factor = 3.0;
	double jump_speed_factor = 1.1;
	double jump_force = 10.0;
	double gravity = -30.0;


	bool on_ground = false;
	bool flying = false;
	bool sneaking = false;
	bool sprinting = false;
	bool jumping = false;

	Transform transform = Transform({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	HitBox hitbox = HitBox({-0.4, 0, -0.4}, {0.8, 1.8, 0.8});
	HitBox feet = HitBox({-0.4, -0.01, -0.4}, {0.8, 0.1, 0.8});

	glm::dvec3 velocity = glm::dvec3(0.0);
	glm::dvec3 input_velocity = glm::dvec3(0.0);

	int jump_remaining = 1;

	double yaw = 0.0;
	double pitch = 0.0;

private:

	std::chrono::nanoseconds fall_start_time = std::chrono::nanoseconds(0);
	glm::dvec3 fall_start_position = glm::dvec3(0.0);

	void updateTransform();
};
