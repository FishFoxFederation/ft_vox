#pragma once

#include "define.hpp"
#include "AThreadWrapper.hpp"
#include "World.hpp"
#include "WorldScene.hpp"
#include "VulkanAPI.hpp"

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
class BlockUpdateThread : public AThreadWrapper
{
public:

	BlockUpdateThread(
		WorldScene & worldScene,
		VulkanAPI & vulkanAPI
	);
	~BlockUpdateThread();

	BlockUpdateThread(BlockUpdateThread& other) = delete;
	BlockUpdateThread(BlockUpdateThread&& other) = delete;
	BlockUpdateThread& operator=(BlockUpdateThread& other) = delete;

private:

	World	m_world;
	WorldScene & m_worldScene;
	VulkanAPI & m_vulkanAPI;

	/**
	 * @brief WIP
	 */
	void init() override;

	/**
	 * @brief WIP
	 */
	void loop() override;
};
