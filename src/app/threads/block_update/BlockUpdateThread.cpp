#include "BlockUpdateThread.hpp"

#include <unistd.h>

BlockUpdateThread::BlockUpdateThread(
	ClientWorld & world
):
	m_world(world),
	m_thread(&BlockUpdateThread::launch, this)
{
}

BlockUpdateThread::~BlockUpdateThread()
{
}

void BlockUpdateThread::launch()
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

void BlockUpdateThread::init()
{
	LOG_INFO("BlockUpdateThread launched :" << gettid());
	tracy::SetThreadName(str_block_update_thread);
}

void BlockUpdateThread::loop()
{
	(void)m_world;
}
