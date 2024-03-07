#pragma once

#include "define.hpp"
#include "AThreadWrapper.hpp"
#include "WorldScene.hpp"
#include "Settings.hpp"
#include "VulkanAPI.hpp"
#include "Chunk.hpp"
#include "WorldGenerator.hpp"
#include "DebugGui.hpp"

#include <chrono>
#include <map>

/**
 * @brief The push constant for the model matrix.
 */
struct ModelMatrix_push_constant
{
	glm::mat4 model;
};


/**
 * @brief A wrapper for the thread that handles rendering
 *
 * This thread will run at max framerate possible
 * Interacts with the vk::RenderAPI to draw the Scene
 * Every function called from this class MUST be thread safe
 */
class RenderThread : public AThreadWrapper
{

public:

	/**
	 * @brief Construct a new RenderThread object
	*/
	RenderThread(
		const Settings & settings,
		VulkanAPI & vulkanAPI,
		const WorldScene & worldScene,
		std::chrono::nanoseconds start_time
	);

	/**
	 * @brief Destroy the RenderThread object
	*/
	~RenderThread();

	RenderThread(RenderThread & renderer) = delete;
	RenderThread(RenderThread && renderer) = delete;
	RenderThread & operator=(RenderThread & renderer) = delete;
	RenderThread & operator=(RenderThread && renderer) = delete;

private:

	const Settings & m_settings;
	VulkanAPI & vk;
	const WorldScene & m_world_scene;

	DebugGui m_debug_gui;

	struct ChunkData
	{
		glm::vec3 position;
		uint64_t mesh_id;
	};

	WorldGenerator world_generator;
	std::vector<std::pair<glm::vec3, Chunk>> m_chunks;
	std::vector<ChunkData> m_chunks_to_draw;


	std::chrono::nanoseconds m_start_time;
	std::chrono::nanoseconds m_current_time;
	std::chrono::nanoseconds m_last_frame_time;
	std::chrono::nanoseconds m_delta_time;


	// For debugging

	int m_frame_count;
	std::chrono::nanoseconds m_start_time_counting_fps;

	std::chrono::nanoseconds m_start_cpu_rendering_time;
	std::chrono::nanoseconds m_end_cpu_rendering_time;



	/**
	 * @brief function used to initialize the vulkan ressources via the renderAPI
	 *
	 * @details will be initialized here:
	 * - the textures
	 * - the pipelines
	 *
	 */
	void init() override;

	/**
	 * @brief the main loop of the thread
	 *
	 */
	void loop() override;

	/**
	 * @brief update the time
	 *
	 */
	void updateTime();

	/**
	 * @brief Use the ImGui library to display debug information
	 *
	 */
	void updateImGui();

};
