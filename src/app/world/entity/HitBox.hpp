#pragma once

#include "define.hpp"
#include "Transform.hpp"

#include <glm/glm.hpp>

#include <vector>

class HitBox
{

public:

	HitBox(
		const glm::dvec3 & position,
		const glm::dvec3 & size
	);
	~HitBox();

	HitBox(const HitBox & other) = default;
	HitBox(HitBox && other) = default;
	HitBox & operator=(const HitBox & other) = delete;
	HitBox & operator=(HitBox && other) = delete;

	glm::dvec3 position; // local position
	glm::dvec3 size;
};

bool isColliding(
	const HitBox & hitbox1,
	const glm::dvec3 & position1,
	const HitBox & hitbox2,
	const glm::dvec3 & position2
);
