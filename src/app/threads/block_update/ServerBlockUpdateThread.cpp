#include "ServerBlockUpdateThread.hpp"

ServerBlockUpdateThread::ServerBlockUpdateThread(
	ServerWorld & world
) : m_world(world), m_thread(&ServerBlockUpdateThread::launch, this)
{
}

ServerBlockUpdateThread::~ServerBlockUpdateThread()
{
}

void ServerBlockUpdateThread::launch()
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
		LOG_ERROR(" BLOCKUPDATE Thread exception: " << e.what());
	}
	LOG_DEBUG("Thread stopped");
}

void ServerBlockUpdateThread::init()
{
	LOG_INFO("ServerBlockUpdateThread launched :" << gettid());
	tracy::SetThreadName(str_block_update_thread);
}

void ServerBlockUpdateThread::loop()
{
	auto start_time = std::chrono::high_resolution_clock::now();
	ZoneScopedN("ServerBlockUpdateThread Loop");

	m_world.update();

	auto end_time = std::chrono::high_resolution_clock::now();

	//if the loop is faster than 50ms, sleep for the remaining time
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	if (duration.count() < 50)
		std::this_thread::sleep_for(std::chrono::milliseconds(50 - duration.count()));
}
