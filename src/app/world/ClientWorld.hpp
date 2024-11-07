
#pragma once

#include "define.hpp"

#include "List.hpp"
#include "Player.hpp"
#include "WorldScene.hpp"
#include "WorldGenerator.hpp"
#include "logger.hpp"
#include "CreateMeshData.hpp"
#include "VulkanAPI.hpp"
#include "Camera.hpp"
#include "World.hpp"
#include "SoundEngine.hpp"
#include "EventManager.hpp"

#include <unordered_map>
#include <shared_mutex>
#include <list>

class ClientWorld : public World
{

public:

	struct PlayerUseResult
	{
		bool hit;
		glm::vec3 block_position;
		ItemInfo::Type used_item;
	};

	ClientWorld(
		WorldScene & world_scene,
		VulkanAPI & vulkan_api,
		// Sound::Engine & sound_engine,
		Event::Manager & event_manager,
		uint64_t my_player_id = 0
	);
	~ClientWorld();

	ClientWorld(ClientWorld & other) = delete;
	ClientWorld(ClientWorld && other) = delete;
	ClientWorld & operator=(ClientWorld & other) = delete;
	ClientWorld & operator=(ClientWorld && other) = delete;


	void updateEntities();
	void updateBlock(glm::dvec3 position);

	int		getRenderDistance() const;
	void	setRenderDistance(const int & render_distance);
	int		getServerLoadDistance() const;
	void	setServerLoadDistance(const int & server_load_distance);
	
	std::pair<glm::dvec3, glm::dvec3> calculatePlayerMovement(
		const uint64_t player_id,
		const int8_t forward,
		const int8_t backward,
		const int8_t left,
		const int8_t right,
		const int8_t up,
		const int8_t down,
		const double delta_time_second
	);
	void updatePlayerCamera(
		const uint64_t player_id,
		const double x_offset,
		const double y_offset
	);
	void updatePlayerTargetBlock(
		const uint64_t player_id
	);
	std::pair<bool, glm::vec3> playerAttack(
		const uint64_t player_id,
		bool attack
	);
	PlayerUseResult playerUse(
		const uint64_t player_id,
		bool use
	);
	void changePlayerViewMode(
		const uint64_t player_id
	);
	void manageScroll(
		const double x_offset,
		const double y_offset
	);
	void updatePlayer(
		const uint64_t player_id,
		std::function<void(Player &)> update
	);

	void createMob();
	void updateMobs(
		const double delta_time_second
	);

	void modifyBlock(
		const glm::vec3 & position,
		const BlockInfo::Type & block_id
	);

	//Server side
	uint64_t	createPlayer(const glm::vec3 & position);

	//Client side
	void		addPlayer(const uint64_t player_id, const glm::dvec3 & position);
	void		removePlayer(const uint64_t player_id);

	void		updatePlayerPosition(const uint64_t & player_id, const glm::dvec3 & position);
	void		applyPlayerMovement(const uint64_t & player_id, const glm::dvec3 & displacement);

	Camera getCamera(const uint64_t player_id);
	glm::dvec3 getPlayerPosition(const uint64_t player_id);


	void					addChunk(std::shared_ptr<Chunk> chunk);
	void					removeChunk(const glm::ivec3 & chunkPosition);
	std::vector<glm::ivec3> getNeededChunks(const glm::vec3 & playerPosition);


	std::shared_ptr<Chunk> localGetChunk(const glm::ivec3 & position) const;

	void 	otherUpdate();

	uint64_t m_my_player_id;
private:

	WorldScene &							m_world_scene;
	VulkanAPI &								m_vulkan_api;
	// Sound::Engine &							m_sound_engine;
	Event::Manager &						m_event_manager;

	int										m_render_distance = 16;
	int										m_server_load_distance = 0;

	/*************************************
	 *  CHUNKS AND MAP
	*************************************/
	std::unordered_set<glm::ivec2>			m_loaded_chunks;
	TracyLockableN							(std::mutex, m_loaded_chunks_mutex, "Loaded Chunks");

	// std::unordered_set<glm::ivec2>			m_visible_chunks;
	// std::mutex								m_visible_chunks_mutex;

	std::unordered_set<glm::ivec2>          m_chunks_to_mesh;
	TracyLockableN							(std::mutex, m_chunks_to_mesh_mutex, "Chunks to mesh");

	std::unordered_set<glm::ivec2> 			m_unload_set;
	TracyLockableN							(std::mutex, m_unload_set_mutex, "Unload Set");

	std::queue<std::pair<glm::vec3, BlockInfo::Type>> m_blocks_to_set;
	TracyLockableN							(std::mutex, m_blocks_to_set_mutex, "Blocks to set");

	std::queue<glm::ivec3> 					m_block_light_update;
	TracyLockableN							(std::mutex, m_block_light_update_mutex, "Block light update");



	/*********************************************************
	 *
	 *  METHODS
	 *
	*********************************************************/


	/*************************************
	 *  CHUNKS AND MAP
	*************************************/

	// /**
	//  * @brief will load all chunks around the player
	//  * @warning you must lock m_chunks_mutex before calling this function
	//  *
	//  * @param playerPosition
	//  */
	// void 	loadChunks(const glm::vec3 & playerPosition);

	// /**
	//  * @brief will load all chunks around the players
	//  * @warning you must lock m_chunks_mutex before calling this function
	//  *
	//  * @param playerPositions
	//  */
	// void 	loadChunks(const std::vector<glm::vec3> & playerPositions);

	// /**
	//  * @brief will unload chunk that are too far from the player
	//  * @warning you must lock m_chunks_mutex and unload_set_mutex before calling this function
	//  * @param playerPosition
	//  */
	// void	unloadChunks(const glm::vec3 & playerPosition);

	// /**
	//  * @brief will unload chunk that are too far from the players
	//  * @warning you must lock m_chunks_mutex and unload_set_mutex before calling this function
	//  *
	//  * @param playerPositions
	//  */
	// void	unloadChunks(const std::vector<glm::vec3> & playerPositions);

	void 	unloadChunk(const glm::ivec3 & chunkPosition);

	/**
	 * @brief will mesh chunks that are meshable around the player
	 * @warning you must lock m_chunks_mutex as well as m_visible_chunks_mutex
	 * before calling this function
	 *
	 * @param playerPosition
	 */
	void	meshChunks(const glm::vec3 & playerPosition);

	/**
	 * @brief will mesh a chunk
	 *
	 * @param chunkPosition
	 */
	void 	meshChunk(const glm::ivec2 & chunkPosition);

	/**
	 * @brief Will load, unload and mesh chunks around the player
	 *
	 * @param playerPosition
	 */
	void 	updateChunks(const glm::vec3 & playerPosition);

	void 	doBlockSets();

	void 	updateLights();

	void	setChunkNotMeshed(const glm::ivec2 & chunkPosition);


	/*************************************
	 *  ENTITIES
	 *************************************/
	bool hitboxCollisionWithBlock(
		const HitBox & hitbox,
		const glm::dvec3 & position,
		const uint64_t block_properties
	);

	/*************************************
	 *  RAYCAST
	 *************************************/
	RayCastOnBlockResult rayCastOnBlock(
		const glm::vec3 & origin,
		const glm::vec3 & direction,
		const double max_distance
	);

};
