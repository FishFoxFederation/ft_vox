#include "Camera.hpp"
#include "DebugGui.hpp"
#include "logger.hpp"

//############################################################################################################
//                                                                                                           #
//                                                ViewFrustum                                                #
//                                                                                                           #
//############################################################################################################

ViewFrustum::ViewFrustum(
	const glm::dvec3 & pos, const glm::dvec3 & front, const glm::dvec3 & up,
	const double fov, const double ratio, const double nearD, const double farD
)
{
	m_ratio = ratio;
	m_nearD = nearD;
	m_farD = farD;
	m_fov = glm::radians(fov);
	m_tang = tanf(m_fov * 0.5);

	// compute sphere factors for sphere intersection test
	m_sphereFactorY = 1.0 / cosf(m_fov);
	m_sphereFactorX = 1.0 / cosf(atanf(m_tang * m_ratio));

	m_camera_position = pos;
	m_z = glm::normalize(front);
	m_x = glm::normalize(glm::cross(m_z, up));
	m_y = glm::cross(m_x, m_z);

	// compute the center of the near and far planes
	glm::dvec3 nc = m_camera_position + m_z * (m_nearD + 0.1);
	glm::dvec3 fc = m_camera_position + m_z * (m_farD - 1.0);

	// compute the size of the near and far planes
	m_nearH = m_nearD * m_tang;
	m_nearW = m_nearH * m_ratio;
	m_farH = m_farD * m_tang;
	m_farW = m_farH * m_ratio;

	// compute the 8 corners of the frustum
	nbr = nc + m_y * m_nearH + m_x * m_nearW;
	nbl = nc + m_y * m_nearH - m_x * m_nearW;
	ntr = nc - m_y * m_nearH + m_x * m_nearW;
	ntl = nc - m_y * m_nearH - m_x * m_nearW;
	fbr = fc + m_y * m_farH + m_x * m_farW;
	fbl = fc + m_y * m_farH - m_x * m_farW;
	ftr = fc - m_y * m_farH + m_x * m_farW;
	ftl = fc - m_y * m_farH - m_x * m_farW;
}

bool ViewFrustum::sphereInFrustum(const glm::dvec3 & center, double radius) const
{
	double d;

	glm::dvec3 v = center - m_camera_position;

	double az = glm::dot(v, m_z);
	if (az > m_farD + radius || az < m_nearD - radius)
	{
		return false;
	}

	double ay = glm::dot(v, m_y);
	double h = az * m_tang;
	d = radius * m_sphereFactorY;
	if (ay > h + d || ay < -h - d)
	{
		return false;
	}

	double ax = glm::dot(v, m_x);
	double w = h * m_ratio;
	d = radius * m_sphereFactorX;
	if (ax > w + d || ax < -w - d)
	{
		return false;
	}

	return true;
}

//############################################################################################################
//                                                                                                           #
//                                                  Camera                                                   #
//                                                                                                           #
//############################################################################################################

void Camera::movePosition(const glm::dvec3 & move)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	glm::dvec3 displacement = move.x * glm::normalize(glm::cross(direction(), up))
							+ move.y * up
							+ move.z * glm::normalize(glm::dvec3(direction().x, 0.0, direction().z));
	position += displacement;
}

void Camera::moveDirection(double x_offset, double y_offset)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// update the pitch and yaw
	pitch = glm::clamp(pitch - y_offset, -89.0, 89.0);
	yaw += x_offset;

}

void Camera::setPosition(const glm::dvec3& position)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	this->position = position;
}

void Camera::lookAt(const glm::dvec3& target)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	glm::dvec3 direction = glm::normalize(target - position);
	pitch = glm::degrees(asin(direction.y));
	yaw = glm::degrees(atan2(direction.z, direction.x));
}

glm::dvec3 Camera::direction() const
{
	return glm::dvec3(
		cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
		sin(glm::radians(pitch)),
		cos(glm::radians(pitch)) * sin(glm::radians(yaw))
	);
}

glm::mat4 Camera::getViewMatrix() const
{
	return glm::lookAt(position, position + direction(), up);
}

glm::mat4 Camera::getProjectionMatrix(double aspect_ratio) const
{
	return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
}

ViewFrustum Camera::getViewFrustum(double aspect_ratio) const
{
	return ViewFrustum(
		position, direction(), up,
		fov, aspect_ratio, near_plane, far_plane
	);
}
