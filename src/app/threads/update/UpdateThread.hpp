#pragma once

#include "define.hpp"
#include "AThreadWrapper.hpp"
#include "window.hpp"
#include "WorldScene.hpp"
#include "Settings.hpp"

#include <chrono>

class UpdateThread : public AThreadWrapper
{
public:

	UpdateThread(
		const Settings & settings,
		Window & window,
		WorldScene & world_scene,
		std::chrono::nanoseconds start_time
	);
	~UpdateThread();

	UpdateThread(UpdateThread& other) = delete;
	UpdateThread(UpdateThread&& other) = delete;
	UpdateThread& operator=(UpdateThread& other) = delete;
	UpdateThread& operator=(UpdateThread&& other) = delete;

private:

	const Settings & m_settings;
	Window & m_window;
	WorldScene & m_world_scene;

	std::chrono::nanoseconds m_start_time;
	std::chrono::nanoseconds m_current_time;
	std::chrono::nanoseconds m_last_frame_time;
	std::chrono::nanoseconds m_delta_time;

	int m_w_key;
	int m_a_key;
	int m_s_key;
	int m_d_key;
	int m_space_key;
	int m_left_shift_key;

	float m_camera_speed = 1.0f;
	float m_camera_sensitivity = 0.05f;
	double m_mouse_x;
	double m_mouse_y;
	double m_last_mouse_x;
	double m_last_mouse_y;

	/**
	 * @brief WIP
	 */
	void init() override;

	/**
	 * @brief WIP
	 */
	void loop() override;

	void updateTime();
	void readInput();
	void moveCamera();
};
