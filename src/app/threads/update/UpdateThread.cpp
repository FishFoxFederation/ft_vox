#include "UpdateThread.hpp"
#include "logger.hpp"

UpdateThread::UpdateThread(
	Window & window,
	WorldScene & world_scene,
	std::chrono::nanoseconds start_time
):
	AThreadWrapper(),
	m_window(window),
	m_world_scene(world_scene),
	m_start_time(start_time),
	m_last_frame_time(start_time)
{
}

UpdateThread::~UpdateThread()
{
	LOG_INFO("UpdateThread::~UpdateThread()");
}

void UpdateThread::init()
{
	LOG_INFO("UpdateThread::init()");
}

void UpdateThread::loop()
{
	updateTime();
	readInput();
	moveCamera();
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
	glm::vec3 move = glm::vec3(0.0f);
	if (m_w_key == GLFW_PRESS)
	{
		move += glm::vec3(0.0f, 0.0f, 1.0f);
	}
	if (m_a_key == GLFW_PRESS)
	{
		move += glm::vec3(-1.0f, 0.0f, 0.0f);
	}
	if (m_s_key == GLFW_PRESS)
	{
		move += glm::vec3(0.0f, 0.0f, -1.0f);
	}
	if (m_d_key == GLFW_PRESS)
	{
		move += glm::vec3(1.0f, 0.0f, 0.0f);
	}
	if (m_space_key == GLFW_PRESS)
	{
		move += glm::vec3(0.0f, 1.0f, 0.0f);
	}
	if (m_left_shift_key == GLFW_PRESS)
	{
		move += glm::vec3(0.0f, -1.0f, 0.0f);
	}
	if (glm::length(move) > 0.0f)
	{
		move = glm::normalize(move) * m_camera_speed * static_cast<float>(m_delta_time.count()) / 1e8f;
		m_world_scene.camera().movePosition(move);
	}

	double x_offset = m_mouse_x - m_last_mouse_x;
	double y_offset = m_last_mouse_y - m_mouse_y;
	m_last_mouse_x = m_mouse_x;
	m_last_mouse_y = m_mouse_y;

	if (m_window.input().isCursorCaptured() && (x_offset != 0.0 || y_offset != 0.0))
	{

		m_world_scene.camera().moveDirection(x_offset, -y_offset);
	}
}