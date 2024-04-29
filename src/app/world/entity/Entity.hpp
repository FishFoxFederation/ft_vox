#pragma once

#include "define.hpp"
#include "Camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>

class Transform
{

public:

	Transform(
		const glm::vec3 & position = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3 & rotation = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3 & scale = glm::vec3(1.0f, 1.0f, 1.0f)
	):
		position(position), rotation(rotation), scale(scale)
	{
	}

	glm::mat4 model() const
	{
		glm::dmat4 model = glm::dmat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::dvec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rotation.y, glm::dvec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotation.z, glm::dvec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);
		return model;
	}

	glm::dvec3 position;
	glm::dvec3 rotation;
	glm::dvec3 scale;

};

class Entity
{

public:

	Entity();
	virtual ~Entity();

	Entity(Entity & other) = delete;
	Entity(Entity && other) = delete;
	Entity & operator=(Entity & other) = delete;
	Entity & operator=(Entity && other) = delete;

	Transform transform;

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

	void movePosition(glm::dvec3 move);

	void moveDirection(double x_offset, double y_offset);

	Camera camera() const
	{
		return Camera(transform.position, m_pitch, m_yaw);
	}

private:

	double m_yaw;
	double m_pitch;

	glm::dvec3 direction() const;

};
