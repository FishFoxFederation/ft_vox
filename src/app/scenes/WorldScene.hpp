#pragma once

#include "define.hpp"
#include "logger.hpp"

#include <cppVulkanAPI.hpp>

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

	struct MeshRenderData
	{
		vk::Mesh::ID id;
		glm::mat4 model;
	};

	class Camera
	{
	
	public:

		/**
		 * @brief Get the view matrix of the camera.
		 * 
		 * @return glm::mat4 
		 */
		glm::mat4 getViewMatrix() const
		{
			return glm::lookAt(position, position + direction(), up);
		}

		/**
		 * @brief Get the projection matrix of the camera.
		 * 
		 * @param aspect_ratio 
		 * @return glm::mat4 
		 */
		glm::mat4 getProjectionMatrix(float aspect_ratio) const
		{
			return glm::perspective(glm::radians(fov), aspect_ratio, 0.1f, 100.0f);
		}

		/**
		 * @brief Move the camera forward/backward on the xz plane.
		 * 
		 * @param distance 
		 */
		void moveForward(float distance)
		{
			position += distance * glm::normalize(glm::vec3(direction().x, 0.0f, direction().z));
		}

		/**
		 * @brief Move the camera right/left on the xz plane.
		 * 
		 * @param distance 
		 */
		void moveRight(float distance)
		{
			position += distance * glm::normalize(glm::cross(direction(), up));
		}

		/**
		 * @brief Move the camera up/down on the y axis.
		 * 
		 * @param distance 
		 */
		void moveUp(float distance)
		{
			position += distance * up;
		}

		/**
		 * @brief Move the camera rotation from the cursor movement.
		 * 
		 * @param x_offset
		 * @param y_offset
		 */
		void moveDirection(float x_offset, float y_offset)
		{
			float sensitivity = 0.2f;

			// update the pitch and yaw
			pitch += -y_offset * sensitivity;
			yaw += x_offset * sensitivity;
		}
	
	private:

		glm::vec3 position{ 0.0f, 0.0f, 0.0f };
		float pitch{ 0.0f };
		float yaw{ 0.0f };
		glm::vec3 up{ 0.0f, 1.0f, 0.0f };
		float fov{ 45.0f };

		glm::vec3 direction() const
		{
			return glm::vec3(
				cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
				sin(glm::radians(pitch)),
				cos(glm::radians(pitch)) * sin(glm::radians(yaw))
			);
		}
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

	/**
	 * @brief Function to add a mesh to the scene.
	 * 
	 * @param meshID The mesh ID.
	 * @param model The model matrix of the mesh.
	 */
	void addMeshData(uint32_t meshID, glm::mat4 model);

	/**
	 * @brief Function to remove a mesh from the scene.
	 * 
	 * @param meshID The mesh ID to remove.
	 */
	void removeMesh(uint32_t meshID);

	/**
	 * @brief Function to get the mesh data.
	 * 
	 * @return std::vector<MeshRenderData> The mesh data.
	 */
	std::vector<MeshRenderData> getMeshRenderData();

	/**
	 * @brief Function to move the camera forward/backward on the xz plane.
	 * 
	 * @param distance 
	 */
	void moveCameraForward(float distance);

	/**
	 * @brief Function to move the camera right/left on the xz plane.
	 * 
	 * @param distance 
	 */
	void moveCameraRight(float distance);

	/**
	 * @brief Function to move the camera up/down on the y axis.
	 * 
	 * @param distance 
	 */
	void moveCameraUp(float distance);

	/**
	 * @brief Function to move the camera rotation from the cursor movement.
	 * 
	 * @param x 
	 * @param y 
	 */
	void moveCameraDirection(float x, float y);

	/**
	 * @brief Function to get the camera.
	 * 
	 * @return Camera A copy of the camera.
	 */
	Camera getCamera();

private:

	std::vector<MeshRenderData> m_mesh_render_data;
	std::mutex m_mesh_render_data_mutex;

	Camera m_camera;
	std::mutex m_camera_mutex;
};