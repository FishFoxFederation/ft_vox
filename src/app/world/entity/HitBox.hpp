#pragma once

#include "define.hpp"

#include <glm/glm.hpp>

#include <vector>

class Projection
{

public:

	Projection(float min, float max):
		min(min), max(max)
	{
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

	const float min;
	const float max;

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
	HitBox(const std::vector<glm::vec3> & vertices);
	~HitBox();

	HitBox(const HitBox & other);
	HitBox(HitBox && other);
	HitBox & operator=(const HitBox & other) = delete;
	HitBox & operator=(HitBox && other) = delete;

	HitBox transform(const glm::mat4 & model) const;

	void insertNormals(std::vector<glm::vec3> & normals) const;


	const std::vector<glm::vec3> vertices;

};

/**
 * @brief check if two hitboxes are colliding using the separating axis theorem
 *
 */
bool isColliding(
	const HitBox & hitbox1,
	const HitBox & hitbox2
);
