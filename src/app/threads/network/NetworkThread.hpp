#pragma once


#include "define.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "RenderAPI.hpp"
#include "DebugGui.hpp"
#include "logger.hpp"
#include "Tracy.hpp"



class NetworkThread
{
public:

	NetworkThread(
		Client & client
	);
	~NetworkThread();

	NetworkThread(NetworkThread& other) = delete;
	NetworkThread(NetworkThread&& other) = delete;
	NetworkThread& operator=(NetworkThread& other) = delete;

private:

	Client	&	m_client;

	std::jthread m_thread;

	/**
	 * @brief function used as the entry point for the thread
	 *
	 */
	void launch();

	void init();

	void loop();
};
