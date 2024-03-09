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

	/**
	 * @brief Check if a sphere is inside the frustum.
	 *
	 * @param center
	 * @param radius
	 * @return true if the sphere is inside the frustum.
	 */
	bool sphereInFrustum(const glm::vec3 & center, float radius) const;

private:

	glm::vec3 m_camera_position;
	glm::vec3 X, Y, Z; // camera axes
	float near_plane, far_plane;
	float horizontal_fov, vertical_fov;
	float aspect_ratio;

	// precalculated values to speed up the sphereInFrustum function
	float sphereFactorX, sphereFactorY;
	
};

/**
 * @brief A simple camera class.
 */
class Camera
{

public:

	Camera() = default;
	~Camera() = default;

	Camera(Camera& camera) = delete;
	Camera(Camera&& camera) = delete;
	Camera& operator=(Camera& camera) = delete;
	Camera& operator=(Camera&& camera) = delete;

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

private:

	glm::vec3 position{ 0.0f, 0.0f, 0.0f };
	float pitch{ 0.0f };
	float yaw{ 0.0f };
	glm::vec3 up{ 0.0f, 1.0f, 0.0f };
	float fov{ 80.0f };
	float far_plane{ 1000.0f };

	mutable std::mutex m_mutex;

	glm::vec3 direction() const;
};