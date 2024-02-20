#pragma once

#include "define.hpp"
#include "AThreadWrapper.hpp"
#include "WorldScene.hpp"

#include <cppVulkanAPI.hpp>

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
		vk::RenderAPI & renderAPI,
		const WorldScene & worldScene
	);

	/**
	 * @brief Destroy the RenderThread object
	*/
	~RenderThread();

	RenderThread(RenderThread& renderer) = delete;
	RenderThread(RenderThread&& renderer) = delete;
	RenderThread & operator=(RenderThread& renderer) = delete;
	RenderThread & operator=(RenderThread&& renderer) = delete;

private:

	vk::RenderAPI & m_renderAPI;

	const WorldScene & m_world_scene;

	uint64_t m_proj_view_ubo_id;
	uint64_t m_texture_id;
	uint64_t m_texture_color_target_id;
	uint64_t m_normal_color_target_id;
	uint64_t m_depth_target_id;
	uint64_t m_simple_shader_pipeline_id;

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

};
