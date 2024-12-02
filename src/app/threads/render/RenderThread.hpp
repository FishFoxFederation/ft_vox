#pragma once

#include "define.hpp"
#include "Settings.hpp"
#include "RenderAPI.hpp"
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
 * Interacts with the RenderAPI to draw the Scene
 * Every function called from this class MUST be thread safe
 */
class RenderThread
{

public:

	/**
	 * @brief Construct a new RenderThread object
	*/
	RenderThread(
		const Settings & settings,
		RenderAPI & render_api,
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

	RenderAPI & m_render_api;

	// Should be the last member
	std::jthread m_thread;

	/**
	 * @brief function used as the entry point for the thread
	 *
	 */
	void launch();

	void init();
	void loop();
};
