#pragma once

#include "define.hpp"
#include "AThreadWrapper.hpp"

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

	BlockUpdateThread();
	~BlockUpdateThread();

	BlockUpdateThread(BlockUpdateThread& other) = delete;
	BlockUpdateThread(BlockUpdateThread&& other) = delete;
	BlockUpdateThread& operator=(BlockUpdateThread& other) = delete;


private:

	/**
	 * @brief this will be empty for now
	 */
	void init() override;

	/**
	 * @brief the main loop of the thread
	 */
	void loop() override;
};
