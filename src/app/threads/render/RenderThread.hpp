#pragma once

#include "define.hpp"
#include "AThreadWrapper.hpp"

#include <cppVulkanAPI.hpp>


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
	RenderThread(vk::RenderAPI & renderAPI);

	/**
	 * @brief Destroy the RenderThread object
	*/
	~RenderThread();

	RenderThread(RenderThread& renderer) = delete;
	RenderThread(RenderThread&& renderer) = delete;
	RenderThread& operator=(RenderThread& renderer) = delete;


private:
	vk::RenderAPI & m_renderAPI;

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
	 * @brief function used to draw the scene
	 *
	 * @details this function will be called at every iteration of the loop
	 *
	 */
	void draw();
};
