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

	Transform transform;
	HitBox hitbox;

};

class LivingEntity: public Entity
{

public:

	LivingEntity();
	~LivingEntity();

	LivingEntity(LivingEntity & other) = delete;
	LivingEntity(LivingEntity && other) = delete;
	LivingEntity & operator=(LivingEntity & other) = delete;
	LivingEntity & operator=(LivingEntity && other) = delete;

	void movePosition(glm::dvec3 move);

	void moveDirection(double x_offset, double y_offset);

	Camera camera() const
	{
		return Camera(transform.position + eyePosition(), m_pitch, m_yaw);
	}

	glm::dvec3 getDisplacement(glm::dvec3 move) const;


private:

	double m_yaw;
	double m_pitch;

	glm::dvec3 eyePosition() const; // the position of the eyes relative to entity position
	glm::dvec3 direction() const;
	void updateTransform();

};
