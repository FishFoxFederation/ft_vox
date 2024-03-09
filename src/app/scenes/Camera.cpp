#include "Camera.hpp"
#include "DebugGui.hpp"

//############################################################################################################
//                                                                                                           #
//                                                  Frustum                                                  #
//                                                                                                           #
//############################################################################################################

Frustum::Frustum(
	const glm::vec3 & camera_position,
	const glm::vec3 & camera_direction,
	const glm::vec3 & camera_up,
	float horizontal_fov,
	float near_plane,
	float far_plane,
	float aspect_ratio
):
	m_camera_position(camera_position),
	near_plane(near_plane),
	far_plane(far_plane),
	aspect_ratio(aspect_ratio)
{
	horizontal_fov = glm::radians(horizontal_fov);
	vertical_fov = 2.0f * atan(tan(horizontal_fov * 0.5f) / aspect_ratio);

	Z = glm::normalize(camera_direction);
	X = glm::normalize(glm::cross(camera_up, Z));
	Y = glm::cross(Z, X);

	// expand the frustum manually because somehow the frustum is too small
	horizontal_fov *= 2.0f;
	vertical_fov *= 2.0f;
}

Frustum::Frustum(Frustum && frustum):
	m_camera_position(frustum.m_camera_position),
	X(frustum.X),
	Y(frustum.Y),
	Z(frustum.Z),
	near_plane(frustum.near_plane),
	far_plane(frustum.far_plane),
	near_width(frustum.near_width),
	near_height(frustum.near_height),
	aspect_ratio(frustum.aspect_ratio),
	sphereFactorX(frustum.sphereFactorX),
	sphereFactorY(frustum.sphereFactorY)
{
}

bool Frustum::sphereInFrustum(const glm::vec3 & center, float radius) const
{
	glm::vec3 toCenter = center - m_camera_position;

	float distanceZ = glm::dot(toCenter, Z);
	if (distanceZ > far_plane + radius || distanceZ < near_plane - radius)
	{
		return false;
	}

	float distanceY = glm::dot(toCenter, Y);
	// height of the frustum at the distanceZ
	float h = distanceZ * tan(vertical_fov * 0.5f);
	float d = radius / cos(vertical_fov * 0.5f);
	if (distanceY > h + d || distanceY < -h - d)
	{
		return false;
	}

	float distanceX = glm::dot(toCenter, X);
	// width of the frustum at the distanceZ
	float w = h * aspect_ratio;
	d = radius / cos(horizontal_fov * 0.5f);
	if (distanceX > w + d || distanceX < -w - d)
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

glm::mat4 Camera::getViewMatrix() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return glm::lookAt(position, position + direction(), up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect_ratio) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return glm::perspective(glm::radians(fov), aspect_ratio, 0.1f, far_plane);
}

Frustum Camera::getFrustum(float aspect_ratio) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return Frustum(position, direction(), up, fov, 0.1f, far_plane, aspect_ratio);
}

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
	pitch += -y_offset;
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
