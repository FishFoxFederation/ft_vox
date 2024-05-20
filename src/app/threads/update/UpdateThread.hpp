#pragma once

#include "define.hpp"
#include "window.hpp"
#include "WorldScene.hpp"
#include "Settings.hpp"
#include "World.hpp"
#include "VulkanAPI.hpp"

#include <chrono>

class UpdateThread
{
public:

	UpdateThread(
		const Settings & settings,
		Window & window,
		WorldScene & world_scene,
		World & world,
		VulkanAPI & vulkan_api,
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
	World & m_world;
	VulkanAPI & m_vulkan_api;

	std::chrono::nanoseconds m_start_time;
	std::chrono::nanoseconds m_current_time;
	std::chrono::nanoseconds m_last_frame_time;
	std::chrono::nanoseconds m_delta_time;

	int m_move_forward;
	int m_move_left;
	int m_move_backward;
	int m_move_right;
	int m_jump;
	int m_sneak;
	int m_attack;
	int m_use;

	double m_mouse_x;
	double m_mouse_y;
	double m_last_mouse_x;
	double m_last_mouse_y;

	std::jthread m_thread;

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
