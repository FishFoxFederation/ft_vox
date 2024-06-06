#pragma once

#include "define.hpp"
#include "logger.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "List.hpp"
#include "Model.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <mutex>
#include <optional>
#include <map>
#include <algorithm>

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
		uint64_t id;
		glm::dmat4 model;
	};

	struct PlayerRenderData
	{
		glm::mat4 model;
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
			m_meshes.push_back({meshID, transform.model()});
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

	/**
	 * @brief Function to set the target block.
	 *
	 * @param target_block The position of the block the camera is looking at. Can be std::nullopt.
	 */
	void setTargetBlock(const std::optional<glm::vec3> & target_block);

	/**
	 * @brief Function to get the target block.
	 *
	 * @return The position of the block the camera is looking at.
	 */
	std::optional<glm::vec3> targetBlock() const;

	void addPlayer(uint64_t player_id, glm::mat4 model);
	void removePlayer(uint64_t player_id);
	void updatePlayer(uint64_t player_id, glm::mat4 model);
	std::vector<PlayerRenderData> getPlayers() const;

	struct DebugBlock
	{
		glm::vec3 position;
		float size;
		glm::vec4 color;
	};

	void setDebugBlock(const std::vector<DebugBlock> & debug_block);
	void clearDebugBlocks();
	std::vector<DebugBlock> debugBlocks() const;


	// MeshList chunk_mesh_list;
	IdList<uint64_t, MeshRenderData> chunk_mesh_list;
	IdList<uint64_t, MeshRenderData> entity_mesh_list;

private:

	Camera m_camera;

	// position of the block the camera is looking at
	std::optional<glm::vec3> m_target_block;
	mutable std::mutex m_target_block_mutex;

	std::vector<DebugBlock> m_debug_block;
	mutable std::mutex m_debug_block_mutex;

	std::map<uint64_t, PlayerRenderData> m_players;
	mutable std::mutex m_player_mutex;
};
