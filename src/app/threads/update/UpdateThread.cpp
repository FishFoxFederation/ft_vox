#include "UpdateThread.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

#include <unistd.h>

UpdateThread::UpdateThread(
	const Settings & settings,
	Window & window,
	WorldScene & world_scene,
	World & world,
	std::chrono::nanoseconds start_time,
	std::exception_ptr & eptr_ref
):
	m_settings(settings),
	m_window(window),
	m_world_scene(world_scene),
	m_world(world),
	m_start_time(start_time),
	m_last_frame_time(start_time),
	m_thread(&UpdateThread::launch, this),
	m_eptr_ref(eptr_ref)
{
	(void)m_start_time;
}

UpdateThread::~UpdateThread()
{
	this->stop();
	this->m_thread.join();
}

void UpdateThread::stop()
{
	this->m_thread.request_stop();
}

void UpdateThread::launch()
{
	try
	{
		init();

		while (!m_thread.get_stop_token().stop_requested())
		{
			loop();
		}
	} catch (std::exception & e) {
		LOG_CRITICAL("UpdateThread exception: " << e.what());
		m_eptr_ref = std::current_exception();
	}
	catch (...)
	{
		LOG_CRITICAL("Update unkown exception");
		m_eptr_ref = std::current_exception();
		m_eptr_ref = std::current_exception();
	}
	m_running = false;
	LOG_INFO("UpdateThread stopped");
}

void UpdateThread::init()
{
	LOG_INFO("UpdateThread launched :" << gettid());
}

void UpdateThread::loop()
{
	updateTime();
	readInput();
	movePlayer();
	m_world.updateBlock(m_world.getPlayerPosition());
}


void UpdateThread::updateTime()
{
	m_current_time = std::chrono::steady_clock::now().time_since_epoch();
	m_delta_time = m_current_time - m_last_frame_time;
	m_last_frame_time = m_current_time;
}

void UpdateThread::readInput()
{
	m_w_key = m_window.input().getKeyState(GLFW_KEY_W);
	m_a_key = m_window.input().getKeyState(GLFW_KEY_A);
	m_s_key = m_window.input().getKeyState(GLFW_KEY_S);
	m_d_key = m_window.input().getKeyState(GLFW_KEY_D);
	m_space_key = m_window.input().getKeyState(GLFW_KEY_SPACE);
	m_left_shift_key = m_window.input().getKeyState(GLFW_KEY_LEFT_SHIFT);

	m_window.input().getCursorPos(m_mouse_x, m_mouse_y);
}

void UpdateThread::movePlayer()
{
	glm::dvec3 move = glm::dvec3(0.0f);
	if (m_w_key == GLFW_PRESS) move.z += 1.0f;
	if (m_a_key == GLFW_PRESS) move.x += -1.0f;
	if (m_s_key == GLFW_PRESS) move.z += -1.0f;
	if (m_d_key == GLFW_PRESS) move.x += 1.0f;
	if (m_space_key == GLFW_PRESS) move.y += 1.0f;
	if (m_left_shift_key == GLFW_PRESS) move.y += -1.0f;

	if (glm::length(move) > 0.0f)
	{
		move = glm::normalize(move);
	}

	move *= m_camera_speed * static_cast<double>(m_delta_time.count()) / 1e9;

	glm::dvec2 look = glm::dvec2(m_mouse_x - m_last_mouse_x, m_mouse_y - m_last_mouse_y) * m_settings.mouseSensitivity();
	m_last_mouse_x = m_mouse_x;
	m_last_mouse_y = m_mouse_y;

	if (!m_window.input().isCursorCaptured())
	{
		look = glm::dvec2(0.0);
	}

	m_world.updatePlayer(move, look);

	m_world_scene.camera() = m_world.getCamera();
}
