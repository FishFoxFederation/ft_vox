#include "UpdateThread.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

#include <unistd.h>

UpdateThread::UpdateThread(
	const Settings & settings,
	Window & window,
	WorldScene & world_scene,
	std::chrono::nanoseconds start_time
):
	AThreadWrapper(),
	m_settings(settings),
	m_window(window),
	m_world_scene(world_scene),
	m_start_time(start_time),
	m_last_frame_time(start_time)
{
	(void)m_start_time;
}

UpdateThread::~UpdateThread()
{
	this->m_thread.request_stop();
	this->m_thread.join();
}

void UpdateThread::init()
{
	LOG_INFO("UpdateThread launched :" << gettid());
}

void UpdateThread::loop()
{
	updateTime();
	readInput();
	moveCamera();
	rotateCamera();
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

void UpdateThread::moveCamera()
{
	glm::dvec3 move = glm::dvec3(0.0f);
	if (m_w_key == GLFW_PRESS)
	{
		move += glm::dvec3(0.0f, 0.0f, 1.0f);
	}
	if (m_a_key == GLFW_PRESS)
	{
		move += glm::dvec3(-1.0f, 0.0f, 0.0f);
	}
	if (m_s_key == GLFW_PRESS)
	{
		move += glm::dvec3(0.0f, 0.0f, -1.0f);
	}
	if (m_d_key == GLFW_PRESS)
	{
		move += glm::dvec3(1.0f, 0.0f, 0.0f);
	}
	if (m_space_key == GLFW_PRESS)
	{
		move += glm::dvec3(0.0f, 1.0f, 0.0f);
	}
	if (m_left_shift_key == GLFW_PRESS)
	{
		move += glm::dvec3(0.0f, -1.0f, 0.0f);
	}
	if (glm::length(move) > 0.0f)
	{
		move = glm::normalize(move) * m_camera_speed * static_cast<double>(m_delta_time.count()) / 1e8;
		m_world_scene.camera().movePosition(move);
	}
}

void UpdateThread::rotateCamera()
{
	double x_offset = m_mouse_x - m_last_mouse_x;
	double y_offset = m_last_mouse_y - m_mouse_y;
	m_last_mouse_x = m_mouse_x;
	m_last_mouse_y = m_mouse_y;

	if (m_window.input().isCursorCaptured() && (x_offset != 0.0 || y_offset != 0.0))
	{
		double sensitivity = m_settings.mouseSensitivity();
		m_world_scene.camera().moveDirection(
			x_offset * sensitivity,
			-y_offset * sensitivity
		);
	}
}
