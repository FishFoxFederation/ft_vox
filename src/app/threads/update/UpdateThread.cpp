#include "UpdateThread.hpp"
#include "logger.hpp"

UpdateThread::UpdateThread(
	Window & window,
	WorldScene & worldScene
):
	AThreadWrapper(),
	m_window(window),
	m_world_scene(worldScene)
{
}

UpdateThread::~UpdateThread()
{
	LOG_INFO("UpdateThread::~UpdateThread()");
}

void UpdateThread::init()
{

}

void UpdateThread::loop()
{
}
