#pragma once

#include "define.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "HitBox.hpp"
#include "Block.hpp"
#include "Item.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Tracy.hpp"

#include <cstdint>
#include <chrono>
#include <mutex>
#include <optional>
#include <array>

struct RayCastOnBlockResult
{
	bool hit;
	glm::vec3 block_position;
	glm::vec3 normal;
	glm::vec3 hit_position;
	BlockInfo::Type block;
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

	Camera camera() const;

	glm::dvec3 getTransformedMovement(glm::dvec3 move) const;

	glm::dvec3 cameraPosition() const; // the position of the eyes relative to entity position
	glm::dvec3 direction() const;

	bool shouldCollide() const;
	bool shouldFall() const;
	bool isFlying() const;

	bool canJump() const;
	void startJump();

	void startFall();
	double fallDuration();

	bool canAttack() const;
	void startAttack();

	bool canUse() const;
	void startUse();

	TracyLockableN (std::mutex,  mutex, "Player internal");

	enum class GameMode
	{
		SURVIVAL,
		CREATIVE,
		SPECTATOR
	} gameMode = GameMode::SPECTATOR;

	enum class ViewMode
	{
		FIRST_PERSON,
		THIRD_PERSON_BACK
	} view_mode = ViewMode::FIRST_PERSON;

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
	bool swimming = false;

	BlockInfo::Type ground_block = BlockInfo::Type::Air;

	Transform transform = Transform({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
	HitBox hitbox = HitBox({-0.3, 0, -0.3}, {0.6, 1.8, 0.6});
	HitBox feet = HitBox({-0.3, -0.01, -0.3}, {0.6, 0.1, 0.6});

	glm::dvec3 velocity = glm::dvec3(0.0);

	int jump_remaining = 1;

	glm::dvec3 eye_position = glm::dvec3(0.0, 1.6, 0.0);
	double yaw = 0.0;
	double pitch = 0.0;

	RayCastOnBlockResult targeted_block{false, glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), BlockInfo::Type::Air, false};

	uint64_t connection_id = 0;
	uint64_t player_id = 0;
	uint64_t player_ticket_id = 0;


	std::array<ItemInfo::Type, 9> toolbar_items = {
		ItemInfo::Type::GrassBlock,
		ItemInfo::Type::DirtBlock,
		ItemInfo::Type::StoneBlock,
		ItemInfo::Type::WaterBlock,
		ItemInfo::Type::GlassBlock,
		ItemInfo::Type::LightBlock,
		ItemInfo::Type::WoodBlock,
		ItemInfo::Type::LeavesBlock,
		ItemInfo::Type::None
	};
	int toolbar_cursor = 0;

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
