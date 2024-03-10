#include "Camera.hpp"
#include "DebugGui.hpp"
#include "logger.hpp"

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
	m_nearD(near_plane),
	m_farD(far_plane),
	m_aspect_ratio(aspect_ratio)
{
	m_horizontal_fov = glm::radians(horizontal_fov);
	m_vertical_fov = 2.0f * atan(tan(m_horizontal_fov * 0.5f) / m_aspect_ratio);

	Z = glm::normalize(camera_direction);
	X = glm::normalize(glm::cross(camera_up, Z));
	Y = glm::cross(Z, X);

	// expand the frustum manually because somehow the frustum is too small
	// horizontal_fov *= 2.0f;
	// vertical_fov *= 2.0f;

	// TODO: calculate the sphereFactorX and sphereFactorY

	m_nearD += 0.1f;
	m_farD -= 0.1f;

	// calculate the 8 corners of the frustum
	float h = tan(m_vertical_fov * 0.5f) * m_nearD;
	float w = h * m_aspect_ratio;

	nbr = m_camera_position + Z * m_nearD + Y * h + X * w;
	nbl = m_camera_position + Z * m_nearD + Y * h - X * w;
	ntr = m_camera_position + Z * m_nearD - Y * h + X * w;
	ntl = m_camera_position + Z * m_nearD - Y * h - X * w;

	h = tan(m_vertical_fov * 0.5f) * m_farD;
	w = h * m_aspect_ratio;

	fbr = m_camera_position + Z * m_farD + Y * h + X * w;
	fbl = m_camera_position + Z * m_farD + Y * h - X * w;
	ftr = m_camera_position + Z * m_farD - Y * h + X * w;
	ftl = m_camera_position + Z * m_farD - Y * h - X * w;

}

Frustum::Frustum(Frustum && frustum):
	m_camera_position(frustum.m_camera_position),
	X(frustum.X),
	Y(frustum.Y),
	Z(frustum.Z),
	m_nearD(frustum.m_nearD),
	m_farD(frustum.m_farD),
	m_aspect_ratio(frustum.m_aspect_ratio),
	sphereFactorX(frustum.sphereFactorX),
	sphereFactorY(frustum.sphereFactorY)
{
}

bool Frustum::pointInFrustum(const glm::vec3 & point) const
{
	glm::vec3 toPoint = point - m_camera_position;

	float distanceZ = glm::dot(toPoint, Z);
	if (distanceZ > m_farD || distanceZ < m_nearD)
	{
		return false;
	}

	float distanceY = glm::dot(toPoint, Y);
	float h = distanceZ * tan(m_vertical_fov * 0.5f);
	if (distanceY > h || distanceY < -h)
	{
		return false;
	}

	float distanceX = glm::dot(toPoint, X);
	float w = h * m_aspect_ratio;
	if (distanceX > w || distanceX < -w)
	{
		return false;
	}

	return true;
}

bool Frustum::boxInFrustum(const glm::vec3 & min, const glm::vec3 & max) const
{
	glm::vec3 vertices[8] = {
		glm::vec3(min.x, min.y, min.z),
		glm::vec3(max.x, min.y, min.z),
		glm::vec3(min.x, max.y, min.z),
		glm::vec3(max.x, max.y, min.z),
		glm::vec3(min.x, min.y, max.z),
		glm::vec3(max.x, min.y, max.z),
		glm::vec3(min.x, max.y, max.z),
		glm::vec3(max.x, max.y, max.z)
	};

	for (int i = 0; i < 8; i++)
	{
		if (pointInFrustum(vertices[i]))
		{
			return true;
		}
	}

	return false;
}

bool Frustum::sphereInFrustum(const glm::vec3 & center, float radius) const
{
	glm::vec3 v = center - m_camera_position;

	float az = glm::dot(v, Z);
	if (az > m_farD + radius || az < m_nearD - radius)
	{
		return false;
	}

	float ay = glm::dot(v, Y);
	// height of the frustum at the az
	float h = az * tan(m_vertical_fov * 0.5f);
	float d = radius / cos(m_vertical_fov * 0.5f);
	if (ay > h + d || ay < -h - d)
	{
		return false;
	}

	float ax = glm::dot(v, X);
	// width of the frustum at the az
	float w = h * m_aspect_ratio;
	d = radius / cos(m_horizontal_fov * 0.5f);
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

glm::mat4 Camera::getViewMatrix() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return glm::lookAt(position, position + direction(), up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect_ratio) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
}

Frustum Camera::getFrustum(float aspect_ratio) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return Frustum(position, direction(), up, fov, near_plane, far_plane, aspect_ratio);
}

ViewFrustum Camera::getViewFrustum(float aspect_ratio) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return ViewFrustum(
		position, direction(), up,
		fov, aspect_ratio, near_plane, far_plane
	);
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
