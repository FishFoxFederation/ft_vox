#pragma once

#include "define.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "HitBox.hpp"
#include "Block.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <chrono>
#include <mutex>
#include <optional>

struct RayCastOnBlockResult
{
	bool hit;
	glm::vec3 block_position;
	glm::vec3 normal;
	glm::vec3 hit_position;
	BlockID block;
	bool inside_block;
};
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

	bool canJump() const;
	void startJump();

	void startFall();
	double fallDuration();

	bool canAttack() const;
	void startAttack();

	bool canUse() const;
	void startUse();

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
	double fly_speed_factor = 30.0;
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

	RayCastOnBlockResult targeted_block{false, glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), Block::Air.id, false};

private:

	std::chrono::nanoseconds fall_start_time = std::chrono::nanoseconds(0);
	glm::dvec3 fall_start_position = glm::dvec3(0.0);

	std::chrono::nanoseconds last_jump_time = std::chrono::steady_clock::now().time_since_epoch();
	std::chrono::milliseconds jump_delai = std::chrono::milliseconds(500);
	std::chrono::milliseconds jump_delai_after_fall = std::chrono::milliseconds(100);

	std::chrono::nanoseconds last_attack_time = std::chrono::steady_clock::now().time_since_epoch();
	std::chrono::milliseconds attack_delai = std::chrono::milliseconds(200);

	std::chrono::nanoseconds last_use_time = std::chrono::steady_clock::now().time_since_epoch();
	std::chrono::milliseconds use_delai = std::chrono::milliseconds(200);

	void updateTransform();
};
