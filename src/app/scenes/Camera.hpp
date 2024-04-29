#pragma once

#include "define.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <mutex>

class ViewFrustum
{

public:

	ViewFrustum(
		const glm::dvec3 & pos, const glm::dvec3 & front, const glm::dvec3 & up,
		const double fov, const double ratio, const double nearD, const double farD
	);

	bool sphereInFrustum(const glm::dvec3 & center, double radius) const;

	glm::dvec3 nbr, nbl, ntr, ntl, fbr, fbl, ftr, ftl;

private:

	glm::dvec3 m_camera_position;
	glm::dvec3 m_x, m_y, m_z;
	double m_nearD, m_farD;
	double m_ratio;
	double m_fov, m_tang;
	double m_nearH, m_nearW;
	double m_farH, m_farW;
	double m_sphereFactorX, m_sphereFactorY;
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
		glm::dvec3 position;
		ViewFrustum view_frustum;
	};

	Camera() = default;
	~Camera() = default;

	Camera(
		const glm::dvec3 & position,
		double pitch,
		double yaw
	):
		position(position),
		pitch(pitch),
		yaw(yaw),
		up(0.0f, 1.0f, 0.0f),
		fov(80.0f),
		near_plane(0.01f),
		far_plane(1000.0f)
	{
	}

	Camera(const Camera & camera) = delete;
	Camera & operator=(Camera & camera) = delete;

	Camera(Camera && camera):
		position(camera.position),
		pitch(camera.pitch),
		yaw(camera.yaw),
		up(camera.up),
		fov(camera.fov),
		near_plane(camera.near_plane),
		far_plane(camera.far_plane)
	{
	}

	Camera & operator=(Camera && camera)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		position = camera.position;
		pitch = camera.pitch;
		yaw = camera.yaw;
		up = camera.up;
		fov = camera.fov;
		near_plane = camera.near_plane;
		far_plane = camera.far_plane;
		return *this;
	}

	/**
	 * @brief Move the camera with a vector. x = right, y = up, z = forward.
	 *
	 * @param move
	 */
	void movePosition(const glm::dvec3 & move);

	/**
	 * @brief Move the camera rotation from the cursor movement.
	 *
	 * @param x_offset x movement of the cursor.
	 * @param y_offset y movement of the cursor.
	 */
	void moveDirection(double x_offset, double y_offset);

	/**
	 * @brief Set the position of the camera.
	 *
	 * @param position
	 */
	void setPosition(const glm::dvec3 & position);

	/**
	 * @brief Set the pitch and yaw of the camera to look at a target.
	 *
	 * @param target
	 */
	void lookAt(const glm::dvec3 & target);

	glm::dvec3 getPosition() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return position;
	}

	double getPitch() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return pitch;
	}

	double getYaw() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return yaw;
	}

	void setPitch(double pitch)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		this->pitch = pitch;
	}

	void setYaw(double yaw)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		this->yaw = yaw;
	}

	RenderInfo getRenderInfo(double aspect_ratio) const
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

	glm::dvec3 position{ 0.0f, 0.0f, 0.0f };
	double pitch{ 0.0f };
	double yaw{ 0.0f };
	glm::dvec3 up{ 0.0f, 1.0f, 0.0f };
	double fov{ 80.0f };
	double near_plane{ 0.01f };
	double far_plane{ 1000.0f };

	mutable std::mutex m_mutex;

	glm::dvec3 direction() const;

	glm::mat4 getViewMatrix() const;

	glm::mat4 getProjectionMatrix(double aspect_ratio) const;

	ViewFrustum getViewFrustum(double aspect_ratio) const;


};
