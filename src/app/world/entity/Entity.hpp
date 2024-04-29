#pragma once

#include "define.hpp"

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
		m_position(position), m_rotation(rotation), m_scale(scale)
	{
	}

	glm::vec3 & position() { return m_position; }
	const glm::vec3 & position() const { return m_position; }

	glm::vec3 & rotation() { return m_rotation; }
	const glm::vec3 & rotation() const { return m_rotation; }

	glm::vec3 & scale() { return m_scale; }
	const glm::vec3 & scale() const { return m_scale; }

	glm::mat4 model() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, m_position);
		model = glm::rotate(model, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, m_scale);
		return model;
	}

private:

	glm::vec3 m_position;
	glm::vec3 m_rotation;
	glm::vec3 m_scale;

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

protected:

	Transform m_transform;

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

private:

};
