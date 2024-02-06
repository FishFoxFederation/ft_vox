#include "RenderThread.hpp"
#include "logger.hpp"

#include <iostream>

RenderThread::RenderThread(vk::RenderAPI& renderAPI):
	AThreadWrapper(),
	m_renderAPI(renderAPI)
{
}

RenderThread::~RenderThread()
{
}

void RenderThread::init()
{

}

void RenderThread::loop()
{
	draw();
}

void RenderThread::draw()
{
	m_renderAPI.startDraw();
	m_renderAPI.endDraw();
}
