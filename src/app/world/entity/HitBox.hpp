#pragma once

#include "define.hpp"
#include "Transform.hpp"

#include <glm/glm.hpp>

#include <vector>

class Projection
{

public:

	Projection(float min, float max):
		min(min), max(max)
	{
	}
	Projection(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal):
		min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::min())
	{
		for (const auto & vertex : vertices)
		{
			float projection = glm::dot(vertex, normal);
			min = std::min(min, projection);
			max = std::max(max, projection);
		}
	}
	~Projection() = default;

	Projection(const Projection & other);
	Projection(Projection && other);
	Projection & operator=(const Projection & other) = delete;
	Projection & operator=(Projection && other) = delete;

	bool isOverlapping(const Projection & other) const
	{
		return min <= other.max && max >= other.min;
	}

	float min;
	float max;

};

/**
 * @brief A hitbox for collision detection
 *
 */
class HitBox
{

public:

	/**
	 * @brief Construct a new Hit Box object
	 *
	 * @param position the position of the hitbox
	 * @param size the size of the hitbox
	 *
	 */
	HitBox(
		const glm::vec3 & position,
		const glm::vec3 & size
	);
	HitBox(
		const glm::vec3 & position,
		const glm::vec3 & size,
		const std::vector<glm::vec3> & vertices
	);
	~HitBox();

	HitBox(const HitBox & other);
	HitBox(HitBox && other);
	HitBox & operator=(const HitBox & other) = delete;
	HitBox & operator=(HitBox && other) = delete;

	std::vector<glm::vec3> transformedVertices() const;

	void insertNormals(std::vector<glm::vec3> & normals) const;

	Transform transform;

private:

	const std::vector<glm::vec3> m_vertices = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(0.0f, 1.0f, 1.0f)
	};

};

/**
 * @brief check if two hitboxes are colliding using the separating axis theorem
 *
 */
// bool isColliding(
// 	const HitBox & hitbox1,
// 	const HitBox & hitbox2
// );

class CubeHitBox
{

public:

	CubeHitBox(
		const glm::dvec3 & position,
		const glm::dvec3 & size
	):
		position(position),
		size(size)
	{
	}
	~CubeHitBox() = default;

	CubeHitBox(const CubeHitBox & other) = default;
	CubeHitBox(CubeHitBox && other) = default;
	CubeHitBox & operator=(const CubeHitBox & other) = delete;
	CubeHitBox & operator=(CubeHitBox && other) = delete;

	glm::dvec3 position; // local position
	glm::dvec3 size;
};

bool isColliding(
	const CubeHitBox & hitbox1,
	const glm::dvec3 & position1,
	const CubeHitBox & hitbox2,
	const glm::dvec3 & position2
);

glm::dvec3 getOverlap(
	const CubeHitBox & hitbox1,
	const glm::dvec3 & position1,
	const CubeHitBox & hitbox2,
	const glm::dvec3 & position2
);
