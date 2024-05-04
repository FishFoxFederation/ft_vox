#pragma once

#include "define.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "HitBox.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>

/**
 * @brief Entity class
 *
 * This class represents an entity in the world
 * The position of the entity is at the bottom center of the hitbox
 *
 */

class Entity
{

public:

	Entity(const HitBox & hitbox);
	virtual ~Entity();

	Entity(Entity & other) = delete;
	Entity(Entity && other) = delete;
	Entity & operator=(Entity & other) = delete;
	Entity & operator=(Entity && other) = delete;

	virtual bool shouldCollide() const
	{
		return true;
	}

	virtual bool shouldFall() const
	{
		return true;
	}

	std::mutex mutex;

	Transform transform;
	HitBox hitbox;

	glm::dvec3 velocity;

};

class Player: public Entity
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
		return Camera(transform.position + eyePosition(), m_pitch, m_yaw);
	}

	glm::dvec3 getDisplacement(glm::dvec3 move) const;

	bool shouldCollide() const override;
	bool shouldFall() const override;

	void startFall();
	double fallDuration();

	enum class GameMode
	{
		SURVIVAL,
		CREATIVE,
		SPECTATOR
	} gameMode;

	double default_speed = 20.0;
	double sprint_factor = 1.5;
	double sneak_factor = 0.3;

	HitBox feet;

	bool on_ground = false;
	bool flying = false;

	glm::dvec3 input_velocity = glm::dvec3(0.0);

private:

	double m_yaw;
	double m_pitch;

	std::chrono::nanoseconds fall_start_time = std::chrono::nanoseconds(0);
	glm::dvec3 fall_start_position = glm::dvec3(0.0);

	glm::dvec3 eyePosition() const; // the position of the eyes relative to entity position
	glm::dvec3 direction() const;
	void updateTransform();

};
