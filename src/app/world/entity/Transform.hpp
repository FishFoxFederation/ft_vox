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
		const glm::vec3 & rotation_origin = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3 & scale = glm::vec3(1.0f, 1.0f, 1.0f),
		const Transform * parent = nullptr
	):
		position(position),
		rotation(rotation),
		rotation_origin(rotation_origin),
		scale(scale),
		parent(parent)
	{
	}

	glm::dmat4 model() const
	{
		glm::dmat4 model = glm::dmat4(1.0f);
		// model = glm::translate(model, position + rotation_origin);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::dvec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rotation.y, glm::dvec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotation.z, glm::dvec3(0.0f, 0.0f, 1.0f));
		// model = glm::translate(model, -rotation_origin);
		model = glm::scale(model, scale);

		if (parent)
		{
			model = parent->model() * model;
		}

		return model;
	}

	glm::dvec3 position;
	glm::dvec3 rotation;
	glm::dvec3 rotation_origin;
	glm::dvec3 scale;

	const Transform * parent;

};
