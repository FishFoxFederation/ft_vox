#include "HitBox.hpp"
#include "logger.hpp"

HitBox::HitBox(
	const glm::dvec3 & position,
	const glm::dvec3 & size
):
	position(position),
	size(size)
{
}

HitBox::~HitBox()
{
}

bool isColliding(
	const HitBox & hitbox1,
	const glm::dvec3 & position1,
	const HitBox & hitbox2,
	const glm::dvec3 & position2
)
{
	return
		position1.x + hitbox1.position.x					< position2.x + hitbox2.position.x + hitbox2.size.x	&&
		position1.x + hitbox1.position.x + hitbox1.size.x	> position2.x + hitbox2.position.x					&&
		position1.y + hitbox1.position.y					< position2.y + hitbox2.position.y + hitbox2.size.y	&&
		position1.y + hitbox1.position.y + hitbox1.size.y	> position2.y + hitbox2.position.y					&&
		position1.z + hitbox1.position.z					< position2.z + hitbox2.position.z + hitbox2.size.z	&&
		position1.z + hitbox1.position.z + hitbox1.size.z	> position2.z + hitbox2.position.z;
}
