#pragma once

#include "define.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <mutex>

class Frustum
{
public:

	Frustum(
		const glm::vec3 & camera_position,
		const glm::vec3 & camera_direction,
		const glm::vec3 & camera_up,
		float horizontal_fov,
		float near_plane,
		float far_plane,
		float aspect_ratio
	);
	~Frustum() = default;

	Frustum(Frustum & frustum) = delete;
	Frustum(Frustum && frustum);
	Frustum & operator=(Frustum & frustum) = delete;
	Frustum & operator=(Frustum && frustum) = delete;

	bool pointInFrustum(const glm::vec3 & point) const;

	bool boxInFrustum(const glm::vec3 & min, const glm::vec3 & max) const;

	bool sphereInFrustum(const glm::vec3 & center, float radius) const;

	glm::vec3 nbr, nbl, ntr, ntl, fbr, fbl, ftr, ftl;

private:

	glm::vec3 m_camera_position;
	glm::vec3 X, Y, Z; // camera axes
	float m_nearD, m_farD;
	float m_horizontal_fov, m_vertical_fov;
	float m_aspect_ratio;

	// precalculated values to speed up the sphereInFrustum function
	float sphereFactorX, sphereFactorY;
	
};

class ViewFrustum
{

public:

	ViewFrustum(
		const glm::vec3 & pos, const glm::vec3 & front, const glm::vec3 & up,
		const float fov, const float ratio, const float nearD, const float farD
	)
	{
		m_ratio = ratio;
		m_nearD = nearD;
		m_farD = farD;
		m_fov = glm::radians(fov);
		m_tang = tanf(m_fov * 0.5f);

		m_nearH = m_nearD * m_tang;
		m_nearW = m_nearH * m_ratio;
		m_farH = m_farD * m_tang;
		m_farW = m_farH * m_ratio;

		// compute sphere factors for sphere intersection test
		m_sphereFactorY = 1.0f / cosf(m_fov);
		// float anglex = atanf(m_tang * m_ratio);
		// m_sphereFactorX = 1.0f / cosf(anglex);
		m_sphereFactorX = 1.0f / cosf(atanf(tanf(m_fov * 0.5f) * m_ratio));

		m_camera_position = pos;
		m_z = glm::normalize(front);
		m_x = glm::normalize(glm::cross(m_z, up));
		m_y = glm::cross(m_x, m_z);

		// compute the center of the near and far planes
		glm::vec3 nc = m_camera_position + m_z * (m_nearD + 0.1f);
		glm::vec3 fc = m_camera_position + m_z * (m_farD - 1.0f);

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

	bool sphereInFrustum(const glm::vec3 & center, float radius) const
	{
		float d;

		glm::vec3 v = center - m_camera_position;

		float az = glm::dot(v, m_z);
		if (az > m_farD + radius || az < m_nearD - radius)
		{
			return false;
		}

		float ay = glm::dot(v, m_y);
		d = radius * m_sphereFactorY;
		az *= m_tang;
		if (ay > az + d || ay < -az - d)
		{
			return false;
		}

		float ax = glm::dot(v, m_x);
		az *= m_ratio;
		d = radius * m_sphereFactorX;
		if (ax > az + d || ax < -az - d)
		{
			return false;
		}

		return true;
	}

	glm::vec3 nbr, nbl, ntr, ntl, fbr, fbl, ftr, ftl;

private:

	glm::vec3 m_camera_position;
	glm::vec3 m_x, m_y, m_z;
	float m_nearD, m_farD;
	float m_ratio;
	float m_fov, m_tang;
	float m_nearH, m_nearW;
	float m_farH, m_farW;
	float m_sphereFactorX, m_sphereFactorY;
};

/**
 * @brief A simple camera class.
 */
class Camera
{

public:

	Camera() = default;
	~Camera() = default;

	Camera(const Camera & camera)
	{
		std::lock_guard<std::mutex> lock(camera.m_mutex);
		position = camera.position;
		pitch = camera.pitch;
		yaw = camera.yaw;
		up = camera.up;
		fov = camera.fov;
		near_plane = camera.near_plane;
		far_plane = camera.far_plane;
	}
	Camera(Camera && camera) = delete;
	Camera & operator=(Camera & camera) = delete;
	Camera & operator=(Camera && camera) = delete;

	/**
	 * @brief Get the view matrix of the camera.
	 *
	 * @return The view matrix.
	 */
	glm::mat4 getViewMatrix() const;

	/**
	 * @brief Get the projection matrix of the camera.
	 *
	 * @param aspect_ratio
	 * @return The projection matrix.
	 */
	glm::mat4 getProjectionMatrix(float aspect_ratio) const;

	/**
	 * @brief Get the frustum of the camera.
	 *
	 * @param aspect_ratio
	 * @return The frustum.
	 */
	Frustum getFrustum(float aspect_ratio) const;

	/**
	 * @brief Get the view frustum of the camera.
	 *
	 * @param aspect_ratio
	 * @return The view frustum.
	 */
	ViewFrustum getViewFrustum(float aspect_ratio) const;

	/**
	 * @brief Move the camera with a vector. x = right, y = up, z = forward.
	 *
	 * @param move
	 */
	void movePosition(const glm::vec3 & move);

	/**
	 * @brief Move the camera rotation from the cursor movement.
	 *
	 * @param x_offset x movement of the cursor.
	 * @param y_offset y movement of the cursor.
	 */
	void moveDirection(float x_offset, float y_offset);

	/**
	 * @brief Set the position of the camera.
	 *
	 * @param position
	 */
	void setPosition(const glm::vec3 & position);

	/**
	 * @brief Set the pitch and yaw of the camera to look at a target.
	 *
	 * @param target
	 */
	void lookAt(const glm::vec3 & target);

	glm::vec3 getPosition() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return position;
	}

	float getPitch() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return pitch;
	}

	float getYaw() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return yaw;
	}

	void setPitch(float pitch)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		this->pitch = pitch;
	}

	void setYaw(float yaw)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		this->yaw = yaw;
	}

private:

	glm::vec3 position{ 0.0f, 0.0f, 0.0f };
	float pitch{ 0.0f };
	float yaw{ 0.0f };
	glm::vec3 up{ 0.0f, 1.0f, 0.0f };
	float fov{ 80.0f };
	float near_plane{ 0.01f };
	float far_plane{ 1000.0f };

	mutable std::mutex m_mutex;

	glm::vec3 direction() const;
};