#include "Camera.hpp"
#include "DebugGui.hpp"
#include "logger.hpp"

//############################################################################################################
//                                                                                                           #
//                                                ViewFrustum                                                #
//                                                                                                           #
//############################################################################################################

ViewFrustum::ViewFrustum(
	const glm::vec3 & pos, const glm::vec3 & front, const glm::vec3 & up,
	const float fov, const float ratio, const float nearD, const float farD
)
{
	m_ratio = ratio;
	m_nearD = nearD;
	m_farD = farD;
	m_fov = glm::radians(fov);
	m_tang = tanf(m_fov * 0.5f);

	// compute sphere factors for sphere intersection test
	m_sphereFactorY = 1.0f / cosf(m_fov);
	m_sphereFactorX = 1.0f / cosf(atanf(m_tang * m_ratio));

	m_camera_position = pos;
	m_z = glm::normalize(front);
	m_x = glm::normalize(glm::cross(m_z, up));
	m_y = glm::cross(m_x, m_z);

	// compute the center of the near and far planes
	glm::vec3 nc = m_camera_position + m_z * (m_nearD + 0.1f);
	glm::vec3 fc = m_camera_position + m_z * (m_farD - 1.0f);

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

bool ViewFrustum::sphereInFrustum(const glm::vec3 & center, float radius) const
{
	float d;

	glm::vec3 v = center - m_camera_position;

	float az = glm::dot(v, m_z);
	if (az > m_farD + radius || az < m_nearD - radius)
	{
		return false;
	}

	float ay = glm::dot(v, m_y);
	float h = az * m_tang;
	d = radius * m_sphereFactorY;
	if (ay > h + d || ay < -h - d)
	{
		return false;
	}

	float ax = glm::dot(v, m_x);
	float w = h * m_ratio;
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

void Camera::movePosition(const glm::vec3 & move)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	glm::vec3 displacement = move.x * glm::normalize(glm::cross(direction(), up))
							+ move.y * up
							+ move.z * glm::normalize(glm::vec3(direction().x, 0.0f, direction().z));

	glm::vec3 last_position = position;
	DebugGui::camera_last_position.set(last_position);
	position += displacement;

	DebugGui::camera_position_sub_last_position.set(position - last_position);

	static std::chrono::nanoseconds last_time = std::chrono::high_resolution_clock::now().time_since_epoch();
	std::chrono::nanoseconds current_time = std::chrono::high_resolution_clock::now().time_since_epoch();
	std::chrono::nanoseconds delta_time = current_time - last_time;
	last_time = current_time;
	DebugGui::camera_update_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(delta_time).count();

	DebugGui::camera_displacement.set(displacement);
	DebugGui::camera_new_position.set(position);
}

void Camera::moveDirection(float x_offset, float y_offset)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// update the pitch and yaw
	pitch = glm::clamp(pitch - y_offset, -89.0f, 89.0f);
	yaw += x_offset;

}

void Camera::setPosition(const glm::vec3& position)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	this->position = position;
}

void Camera::lookAt(const glm::vec3& target)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	glm::vec3 direction = glm::normalize(target - position);
	pitch = glm::degrees(asin(direction.y));
	yaw = glm::degrees(atan2(direction.z, direction.x));
}

glm::vec3 Camera::direction() const
{
	return glm::vec3(
		cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
		sin(glm::radians(pitch)),
		cos(glm::radians(pitch)) * sin(glm::radians(yaw))
	);
}

glm::mat4 Camera::getViewMatrix() const
{
	return glm::lookAt(position, position + direction(), up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect_ratio) const
{
	return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
}

ViewFrustum Camera::getViewFrustum(float aspect_ratio) const
{
	return ViewFrustum(
		position, direction(), up,
		fov, aspect_ratio, near_plane, far_plane
	);
}
