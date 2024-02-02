#include "RenderThread.hpp"

#include <iostream>

RenderThread::RenderThread(vk::RenderAPI& renderAPI)
	:m_renderAPI(renderAPI)
{
}

RenderThread::~RenderThread()
{
}

void RenderThread::draw()
{
	m_renderAPI.startDraw();
	m_renderAPI.endDraw();
}

void RenderThread::init()
{
}

void RenderThread::loop()
{
	draw();
}
