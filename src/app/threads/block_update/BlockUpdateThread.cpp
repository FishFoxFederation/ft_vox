#include "BlockUpdateThread.hpp"

BlockUpdateThread::~BlockUpdateThread()
{
}

void BlockUpdateThread::init()
{

}

void BlockUpdateThread::loop()
{
	while (this->m_thread.get_stop_token().stop_requested() == false)
	{
		// Update blocks
	}
}
