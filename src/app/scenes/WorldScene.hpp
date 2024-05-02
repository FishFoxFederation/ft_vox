#pragma once

#include "define.hpp"
#include "logger.hpp"
#include "Camera.hpp"
#include "List.hpp"

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

	class MeshList
	{

	public:

		MeshList() = default;
		~MeshList() = default;

		MeshList(const MeshList & other) = delete;
		MeshList(MeshList && other) = delete;
		MeshList & operator=(const MeshList & other) = delete;
		MeshList & operator=(MeshList && other) = delete;

		void add(uint64_t meshID, const Transform & transform)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_meshes.push_back({meshID, transform});
		}

		void remove(uint64_t meshID)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			std::erase_if(m_meshes, [meshID](const MeshRenderData & data) { return data.id == meshID; });
		}

		std::vector<MeshRenderData> get() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_meshes;
		}

	private:

		std::vector<MeshRenderData> m_meshes;
		mutable std::mutex m_mutex;
	};

	/**
	 * @brief Construct a new WorldScene object
	 */
	WorldScene();

	/**
	 * @brief Destroy the WorldScene object
	 */
	~WorldScene();

	WorldScene(WorldScene & scene) = delete;
	WorldScene(WorldScene && scene) = delete;
	WorldScene & operator=(WorldScene & scene) = delete;
	WorldScene & operator=(WorldScene && scene) = delete;

	/**
	 * @brief Function to get the camera.
	 *
	 * @return A reference to the camera.
	 */
	Camera & camera() { return m_camera; }

	/**
	 * @brief Function const to get the camera.
	 *
	 * @return A const reference to the camera.
	 *
	 */
	const Camera & camera() const { return m_camera; }

	// MeshList chunk_mesh_list;
	IdList<uint64_t, MeshRenderData> chunk_mesh_list;
	MeshList entity_mesh_list;
	mutable std::mutex mesh_list_mutex;

private:

	Camera m_camera;
};
