#pragma once

#include "define.hpp"
#include "World.hpp"
#include "WorldScene.hpp"
#include "VulkanAPI.hpp"
#include "ThreadPool.hpp"
#include "DebugGui.hpp"

/**
 * @brief An implementation of the thread wrapper for the thread that handles block updates
 *
 * @details This thread will run at 20 ticks per second
 * It is in charge of updating blocks
 * As well a loading/unloading chunks
 * And of course generating new chunks
 * every function called from the loop and init function MUST be thread safe
 *
 */
class BlockUpdateThread
{
public:

	BlockUpdateThread(
		WorldScene & worldScene,
		World & world
	);
	~BlockUpdateThread();

	BlockUpdateThread(BlockUpdateThread& other) = delete;
	BlockUpdateThread(BlockUpdateThread&& other) = delete;
	BlockUpdateThread& operator=(BlockUpdateThread& other) = delete;

private:

	WorldScene	&	m_worldScene;
	World		&	m_world;

	std::jthread m_thread;

	/**
	 * @brief function used as the entry point for the thread
	 *
	 */
	void launch();

	/**
	 * @brief WIP
	 */
	void init();

	/**
	 * @brief WIP
	 */
	void loop();
};
