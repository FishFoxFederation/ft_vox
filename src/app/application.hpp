#pragma once

#include "define.hpp"
#include "window.hpp"
#include "Settings.hpp"
#include "ClientWorld.hpp"
#include "RenderThread.hpp"
#include "UpdateThread.hpp"
#include "VulkanAPI.hpp"
#include "BlockUpdateThread.hpp"
#include "NetworkThread.hpp"
#include "Client.hpp"
#include "SoundEngine.hpp"
#include "EventManager.hpp"

#include "Tracy.hpp"
#include "tracy_globals.hpp"
#include <chrono>

class Application
{

public:

	Application(const int & player_id, const std::string & ip_address, const int & port);
	~Application();

	void run();

private:

	std::chrono::nanoseconds m_start_time;

	Client				m_client;
	Settings			m_settings;
	Window				m_window;
	VulkanAPI			m_vulkan_api;
	Event::Manager		m_event_manager;
	Sound::Engine		m_sound_engine;
	ClientWorld			m_world;
	RenderThread		m_render_thread;
	UpdateThread		m_update_thread;
	BlockUpdateThread	m_block_update_thread;
	NetworkThread		m_network_thread;

};
