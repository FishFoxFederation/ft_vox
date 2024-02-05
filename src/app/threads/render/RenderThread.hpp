#pragma once

#include "define.hpp"

#include <thread>
#define VULKAN_INCLUDE_GLFW
#include <cppVulkanAPI.hpp>

#include "AThreadWrapper.hpp"


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
	 * @brief this will be empty for now
	 * 
	 */
	void init() override;

	/**
	 * @brief the main loop of the thread
	 * 
	 */
	void loop() override;

	void draw();
};
