#pragma once

#include "define.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <mutex>
#include "Tracy.hpp"

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
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
		glm::dvec3 position = glm::dvec3(0.0f, 0.0f, 0.0f);
		ViewFrustum view_frustum = ViewFrustum(
			glm::dvec3(0.0f, 0.0f, 0.0f),
			glm::dvec3(0.0f, 0.0f, 0.0f),
			glm::dvec3(0.0f, 0.0f, 0.0f),
			0.0f, 0.0f, 0.0f, 0.0f
		);
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
		far_plane(100.0f)
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
		std::lock_guard lock(m_mutex);
		position = camera.position;
		pitch = camera.pitch;
		yaw = camera.yaw;
		up = camera.up;
		fov = camera.fov;
		near_plane = camera.near_plane;
		far_plane = camera.far_plane;
		return *this;
	}

	RenderInfo getRenderInfo(double aspect_ratio) const
	{
		std::lock_guard lock(m_mutex);
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
	double far_plane{ 100.0f };

	mutable TracyLockableN(std::mutex, m_mutex, "Camera Mutex");

	glm::dvec3 direction() const;

	glm::mat4 getViewMatrix() const;

	glm::mat4 getProjectionMatrix(double aspect_ratio) const;

	ViewFrustum getViewFrustum(double aspect_ratio) const;


};
