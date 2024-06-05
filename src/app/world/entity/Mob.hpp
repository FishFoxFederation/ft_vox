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

class Mob
{

public:

	Mob();
	~Mob();

	Mob(Mob & other) = delete;
	Mob(Mob && other) = delete;
	Mob & operator=(Mob & other) = delete;
	Mob & operator=(Mob && other) = delete;

	bool canJump() const;
	void startJump();

	void startFall();
	double fallDuration();

	std::mutex mutex;

	Transform transform = Transform({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	HitBox hitbox = HitBox({-0.4, 0, -0.4}, {0.8, 1.8, 0.8});
	HitBox feet = HitBox({-0.4, -0.01, -0.4}, {0.8, 0.1, 0.8});

	glm::dvec3 velocity = glm::dvec3(0.0);

	bool on_ground = false;
	bool jumping = false;

	int jump_remaining = 1;

	glm::dvec3 target_pos = glm::dvec3(0.0);

private:

	std::chrono::time_point<std::chrono::steady_clock> fall_start_time;
	glm::dvec3 fall_start_position;

	// std::chrono::nanoseconds last_jump_time = std::chrono::steady_clock::now().time_since_epoch();
	// std::chrono::milliseconds jump_delai = std::chrono::milliseconds(500);
	// std::chrono::milliseconds jump_delai_after_fall = std::chrono::milliseconds(100);

};
