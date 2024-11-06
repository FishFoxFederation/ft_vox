#pragma once

#include "define.hpp"
#include "window.hpp"
#include "WorldScene.hpp"
#include "Settings.hpp"
#include "ClientWorld.hpp"
#include "VulkanAPI.hpp"
#include "Client.hpp"
#include "ClientPacketHandler.hpp"
#include "SoundEngine.hpp"
#include "EventManager.hpp"
#include <chrono>
#include "Tracy.hpp"
#include "tracy_globals.hpp"

class UpdateThread
{
public:

	UpdateThread(
		Client & client,
		const Settings & settings,
		Window & window,
		WorldScene & world_scene,
		ClientWorld & world,
		VulkanAPI & vulkan_api,
		Sound::Engine & sound_engine,
		Event::Manager & event_manager,
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
	ClientWorld & m_world;
	VulkanAPI & m_vulkan_api;
	Client & m_client;
	ClientPacketHandler m_packet_handler;
	Sound::Engine & m_sound_engine;
	Event::Manager & m_event_manager;

	std::chrono::nanoseconds m_start_time;
	std::chrono::nanoseconds m_current_time;
	std::chrono::nanoseconds m_last_frame_time;
	std::chrono::nanoseconds m_delta_time;

	// For DebugGui
	int m_update_count;
	std::chrono::nanoseconds m_start_time_counting_ups;

	int m_move_forward = 0;
	int m_move_left = 0;
	int m_move_backward = 0;
	int m_move_right = 0;
	int m_jump = 0;
	int m_sneak = 0;
	int m_attack = 0;
	int m_use = 0;

	double m_mouse_x;
	double m_mouse_y;
	double m_last_mouse_x;
	double m_last_mouse_y;

	std::chrono::nanoseconds m_last_target_block_update_time = std::chrono::nanoseconds(0);
	std::chrono::milliseconds m_target_block_update_interval = std::chrono::milliseconds(10);

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
	void handlePackets();
};
