#include "Entity.hpp"
#include "logger.hpp"

//############################################################################################################
//                                                                                                           #
//                                                  Entity                                                   #
//                                                                                                           #
//############################################################################################################

Entity::Entity(
	const CubeHitBox & hitbox
):
	hitbox(hitbox)
{
}

Entity::~Entity()
{
}


//############################################################################################################
//                                                                                                           #
//                                                  LivingEntity                                                   #
//                                                                                                           #
//############################################################################################################

LivingEntity::LivingEntity():
	Entity(CubeHitBox({-0.5, 0, -0.5}, {1, 1, 1})),
	m_yaw(0.0),
	m_pitch(0.0)
{
}

LivingEntity::~LivingEntity()
{
}

void LivingEntity::movePosition(glm::dvec3 displacement)
{
	transform.position += displacement;
}

void LivingEntity::moveDirection(double x_offset, double y_offset)
{
	m_yaw += x_offset;
	m_pitch = glm::clamp(m_pitch - y_offset, -89.0, 89.0);

	// updateTransform();
}

glm::dvec3 LivingEntity::direction() const
{
	return glm::normalize(glm::dvec3(
		cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw)),
		sin(glm::radians(m_pitch)),
		cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw))
	));
}

glm::dvec3 LivingEntity::getDisplacement(glm::dvec3 move) const
{
	static const glm::dvec3 up_vec = glm::dvec3(0.0, 1.0, 0.0);
	const glm::dvec3 dir_vec = direction();
	glm::dvec3 displacement = move.x * glm::normalize(glm::cross(dir_vec, up_vec))
							+ move.y * up_vec
							+ move.z * glm::normalize(glm::dvec3(dir_vec.x, 0.0, dir_vec.z));
	return displacement;
}

glm::dvec3 LivingEntity::eyePosition() const
{
	double eye_height = 1.6;
	// First person view
	// return glm::dvec3(0.0, eye_height, 0.0);

	// Third person view
	const double distance = 5;
	const glm::dvec3 dir_vec = direction();
	return glm::dvec3(0.0, eye_height, 0.0) - distance * dir_vec;
}

void LivingEntity::updateTransform()
{
	// update rotation
	transform.rotation.y = -glm::radians(m_yaw);
}
