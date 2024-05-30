#include "NetworkThread.hpp"

NetworkThread::NetworkThread(
	Client & client
) :
	m_client(client),
	m_thread(&NetworkThread::launch, this)
{
}

NetworkThread::~NetworkThread()
{
}

void NetworkThread::launch()
{
	try
	{
		init();

		while (!m_thread.get_stop_token().stop_requested()) 
		{
			loop();
		}
	}
	catch (const std::exception & e)
	{
		LOG_ERROR("NetworkThread exception: " << e.what());
	}
	LOG_DEBUG("NetworkThread stopped");
}

void NetworkThread::init()
{
	LOG_INFO("NetworkThread launched :" << gettid());
}

void NetworkThread::loop()
{
	m_client.runOnce();
}
