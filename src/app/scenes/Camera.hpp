#pragma once

#include "define.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <mutex>

class ViewFrustum
{

public:

	ViewFrustum(
		const glm::vec3 & pos, const glm::vec3 & front, const glm::vec3 & up,
		const float fov, const float ratio, const float nearD, const float farD
	);

	bool sphereInFrustum(const glm::vec3 & center, float radius) const;

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

	struct RenderInfo
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec3 position;
		ViewFrustum view_frustum;
	};

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

	RenderInfo getRenderInfo(float aspect_ratio) const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return {
			getViewMatrix(),
			getProjectionMatrix(aspect_ratio),
			position,
			getViewFrustum(aspect_ratio)
		};
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

	glm::mat4 getViewMatrix() const;

	glm::mat4 getProjectionMatrix(float aspect_ratio) const;

	ViewFrustum getViewFrustum(float aspect_ratio) const;


};