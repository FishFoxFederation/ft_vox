#include "RenderThread.hpp"
#include "logger.hpp"
#include "Model.hpp"
#include "ft_format.hpp"

#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <unistd.h>

RenderThread::RenderThread(
	const Settings & settings,
	VulkanAPI & vulkan_api,
	std::chrono::nanoseconds start_time
):
	vk(vulkan_api),
	m_thread(&RenderThread::launch, this)
{
}

RenderThread::~RenderThread()
{
}

void RenderThread::launch()
{
	try
	{
		init();

		while (!m_thread.get_stop_token().stop_requested())
		{
			// auto start_time = std::chrono::high_resolution_clock::now();
			loop();
			// auto end_time = std::chrono::high_resolution_clock::now();

			// // i want 60fps so if not enough time passed wait a bit
			// std::chrono::nanoseconds frame_time = end_time - start_time;
			// if (frame_time < std::chrono::nanoseconds(16666666))
			// {
			// 	std::this_thread::sleep_for(std::chrono::nanoseconds(16666666) - frame_time);
			// }
		}
	}
	catch (const std::exception & e)
	{
		LOG_ERROR("RENDER THREAD Thread exception: " << e.what());
	}
	LOG_DEBUG("Thread stopped");
}

void RenderThread::init()
{
	LOG_INFO("RenderThread launched :" << gettid());
	tracy::SetThreadName(str_render_thread);
}

void RenderThread::loop()
{
	vk.renderFrame();
}
