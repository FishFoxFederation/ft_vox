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
		const glm::vec3 & scale = glm::vec3(1.0f, 1.0f, 1.0f),
		const Transform * parent = nullptr
	):
		position(position),
		rotation(rotation),
		scale(scale),
		parent(parent)
	{
	}

	glm::dmat4 model() const
	{
		glm::dmat4 model = glm::dmat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::dvec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rotation.y, glm::dvec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotation.z, glm::dvec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);

		if (parent)
		{
			model = parent->model() * model;
		}

		return model;
	}

	glm::dvec3 position;
	glm::dvec3 rotation;
	glm::dvec3 scale;

	const Transform * parent;

};

class Mat4
{

public:

	Mat4(const glm::dmat4 & mat = glm::dmat4(1.0)):
		m_mat(mat)
	{
	}

	glm::dmat4 dmat() const
	{
		return m_mat;
	}

	glm::mat4 mat() const
	{
		return glm::mat4(m_mat);
	}

	Mat4 & translate(const glm::dvec3 & vec)
	{
		m_mat = glm::translate(m_mat, vec);
		return *this;
	}

	Mat4 & rotate(double angle, const glm::dvec3 & axis)
	{
		m_mat = glm::rotate(m_mat, angle, axis);
		return *this;
	}

	Mat4 & scale(const glm::dvec3 & vec)
	{
		m_mat = glm::scale(m_mat, vec);
		return *this;
	}

private:

	glm::dmat4 m_mat;
};
