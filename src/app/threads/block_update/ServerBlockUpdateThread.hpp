#pragma once

#include "define.hpp"
#include "Tracy.hpp"
#include "tracy_globals.hpp"
#include "ServerWorld.hpp"
#include "ThreadPool.hpp"

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
class ServerBlockUpdateThread
{
public:

	ServerBlockUpdateThread(
		ServerWorld & world
	);
	~ServerBlockUpdateThread();

	ServerBlockUpdateThread(ServerBlockUpdateThread& other) = delete;
	ServerBlockUpdateThread(ServerBlockUpdateThread&& other) = delete;
	ServerBlockUpdateThread& operator=(ServerBlockUpdateThread& other) = delete;

private: 

	ServerWorld		&	m_world;

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
