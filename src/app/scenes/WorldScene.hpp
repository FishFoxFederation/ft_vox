#pragma once

#include "define.hpp"
#include "logger.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <mutex>

/**
 * @brief Class to hold the world scene. The instance of this class will be
 * accessed by multiple threads so it will include synchronization logic.
 *
 * @details The WorldScene class will hold the mesh data and the camera data.
 */
class WorldScene
{

public:

	class Transform
	{
	public:
		Transform(
			const glm::vec3 & position = glm::vec3(0.0f, 0.0f, 0.0f),
			const glm::vec3 & rotation = glm::vec3(0.0f, 0.0f, 0.0f),
			const glm::vec3 & scale = glm::vec3(1.0f, 1.0f, 1.0f)
		):
			m_position(position), m_rotation(rotation), m_scale(scale)
		{
		}

		glm::vec3 & position() { return m_position; }
		const glm::vec3 & position() const { return m_position; }

		glm::vec3 & rotation() { return m_rotation; }
		const glm::vec3 & rotation() const { return m_rotation; }

		glm::vec3 & scale() { return m_scale; }
		const glm::vec3 & scale() const { return m_scale; }

		glm::mat4 model() const
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, m_position);
			model = glm::rotate(model, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, m_scale);
			return model;
		}

	private:

		glm::vec3 m_position;
		glm::vec3 m_rotation;
		glm::vec3 m_scale;
	
	};

	struct MeshRenderData
	{
		uint64_t id;
		Transform transform;
	};

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
		 * @brief Move the camera forward/backward on the xz plane.
		 *
		 * @param distance
		 */
		void moveForward(float distance);

		/**
		 * @brief Move the camera right/left on the xz plane.
		 *
		 * @param distance
		 */
		void moveRight(float distance);

		/**
		 * @brief Move the camera up/down on the y axis.
		 *
		 * @param distance
		 */
		void moveUp(float distance);

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
		float fov{ 45.0f };
		float far_plane{ 1000.0f };

		mutable std::mutex m_mutex;

		glm::vec3 direction() const;
	};

	/**
	 * @brief Construct a new WorldScene object
	 */
	WorldScene();

	/**
	 * @brief Destroy the WorldScene object
	 */
	~WorldScene();

	WorldScene(WorldScene& scene) = delete;
	WorldScene(WorldScene&& scene) = delete;
	WorldScene& operator=(WorldScene& scene) = delete;
	WorldScene& operator=(WorldScene&& scene) = delete;

	/**
	 * @brief Function to add a mesh to the scene.
	 *
	 * @param meshID The mesh ID.
	 * @param model The model matrix of the mesh.
	 */
	void addMeshData(uint64_t meshID, const Transform & transform);

	/**
	 * @brief Function to remove a mesh from the scene.
	 *
	 * @param meshID The mesh ID to remove.
	 */
	void removeMesh(uint64_t meshID);

	/**
	 * @brief Function to get the mesh data.
	 *
	 * @return std::vector<MeshRenderData> The mesh data.
	 */
	std::vector<MeshRenderData> getMeshRenderData() const;

	/**
	 * @brief Function to get the camera.
	 *
	 * @return A reference to the camera.
	 */
	Camera& camera() { return m_camera; }

	/**
	 * @brief Function const to get the camera.
	 *
	 * @return A const reference to the camera.
	 *
	 */
	const Camera& camera() const { return m_camera; }

private:

	std::vector<MeshRenderData> m_mesh_render_data;
	mutable std::mutex m_mesh_render_data_mutex;

	Camera m_camera;
};
