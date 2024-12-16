#pragma once

#include "define.hpp"
#include "window.hpp"
#include "WorldScene.hpp"
#include "Settings.hpp"
#include "World.hpp"

#include <chrono>
#include <exception>

class UpdateThread
{
public:

	UpdateThread(
		const Settings & settings,
		Window & window,
		WorldScene & world_scene,
		World & world,
		std::chrono::nanoseconds start_time,
		std::exception_ptr & eptr_ref
	);
	~UpdateThread();

	bool running() const {return m_running;}
	void stop();

	UpdateThread(UpdateThread& other) = delete;
	UpdateThread(UpdateThread&& other) = delete;
	UpdateThread& operator=(UpdateThread& other) = delete;
	UpdateThread& operator=(UpdateThread&& other) = delete;

private:

	const Settings & m_settings;
	Window & m_window;
	WorldScene & m_world_scene;
	World & m_world;

	std::atomic<bool> m_running = true;

	std::chrono::nanoseconds m_start_time;
	std::chrono::nanoseconds m_current_time;
	std::chrono::nanoseconds m_last_frame_time;
	std::chrono::nanoseconds m_delta_time;

	std::jthread m_thread;
	std::exception_ptr & m_eptr_ref;
	
	int m_w_key;
	int m_a_key;
	int m_s_key;
	int m_d_key;
	int m_space_key;
	int m_left_shift_key;

	double m_camera_speed = 20.0f;
	double m_mouse_x;
	double m_mouse_y;
	double m_last_mouse_x;
	double m_last_mouse_y;


	/**
	 * @brief function used as the entry point for the thread
	 *
	 */
	void launch();

	/**
	 * @brief WIP
	 */
	void init();

	/**
	 * @brief WIP
	 */
	void loop();

	void updateTime();
	void readInput();

	void movePlayer();
};
