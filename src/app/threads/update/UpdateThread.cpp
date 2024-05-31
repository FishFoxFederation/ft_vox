#include "UpdateThread.hpp"
#include "logger.hpp"
#include "DebugGui.hpp"

#include <unistd.h>

UpdateThread::UpdateThread(
	Client & client,
	const Settings & settings,
	Window & window,
	WorldScene & world_scene,
	World & world,
	VulkanAPI & vulkan_api,
	std::chrono::nanoseconds start_time
):
	m_client(client),
	m_settings(settings),
	m_window(window),
	m_world_scene(world_scene),
	m_world(world),
	m_vulkan_api(vulkan_api),
	m_client(client),
	m_packet_handler(client, world),
	m_start_time(start_time),
	m_last_frame_time(start_time),
	m_update_count(0),
	m_start_time_counting_ups(start_time),
	m_thread(&UpdateThread::launch, this)
{
	(void)m_vulkan_api;
	(void)m_start_time;
}

UpdateThread::~UpdateThread()
{
	this->m_thread.request_stop();
	this->m_thread.join();
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
	}
	catch (const std::exception & e)
	{
		LOG_ERROR("UPDATE THREAD Thread exception: " << e.what());
	}
	LOG_DEBUG("Thread stopped");
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
	handlePackets();
}

void UpdateThread::updateTime()
{
	m_current_time = std::chrono::steady_clock::now().time_since_epoch();
	m_delta_time = m_current_time - m_last_frame_time;
	m_last_frame_time = m_current_time;

	m_update_count++;
	if (m_current_time - m_start_time_counting_ups >= std::chrono::seconds(1))
	{
		DebugGui::ups = static_cast<double>(m_update_count) / std::chrono::duration_cast<std::chrono::seconds>(m_current_time - m_start_time_counting_ups).count();
		m_update_count = 0;
		m_start_time_counting_ups = m_current_time;
	}
}

void UpdateThread::readInput()
{
	int move_forward_key_status = m_window.input().getKeyState(GLFW_KEY_W);
	int move_left_key_status = m_window.input().getKeyState(GLFW_KEY_A);
	int move_backward_key_status = m_window.input().getKeyState(GLFW_KEY_S);
	int move_right_key_status = m_window.input().getKeyState(GLFW_KEY_D);
	int jump_key_status = m_window.input().getKeyState(GLFW_KEY_SPACE);
	int sneak_key_status = m_window.input().getKeyState(GLFW_KEY_LEFT_SHIFT);
	int attack_key_status = m_window.input().getMouseButtonState(GLFW_MOUSE_BUTTON_LEFT);
	int use_key_status = m_window.input().getMouseButtonState(GLFW_MOUSE_BUTTON_RIGHT);

	m_move_forward = (move_forward_key_status == GLFW_PRESS) ? 1 : 0;
	m_move_left = (move_left_key_status == GLFW_PRESS) ? 1 : 0;
	m_move_backward = (move_backward_key_status == GLFW_PRESS) ? 1 : 0;
	m_move_right = (move_right_key_status == GLFW_PRESS) ? 1 : 0;
	m_jump = (jump_key_status == GLFW_PRESS) ? 1 : 0;
	m_sneak = (sneak_key_status == GLFW_PRESS) ? 1 : 0;
	m_attack = (attack_key_status == GLFW_PRESS) ? 1 : 0;
	m_use = (use_key_status == GLFW_PRESS) ? 1 : 0;

	m_world.playerAttack(m_world.m_my_player_id, m_attack);
	m_world.playerUse(m_world.m_my_player_id, m_use);

	int reset = m_window.input().getKeyState(GLFW_KEY_R);
	int gamemode_0 = m_window.input().getKeyState(GLFW_KEY_0);
	int gamemode_1 = m_window.input().getKeyState(GLFW_KEY_1);
	int gamemode_2 = m_window.input().getKeyState(GLFW_KEY_2);
	int fly = m_window.input().getKeyState(GLFW_KEY_F);

	m_world.updatePlayer(
		m_world.m_my_player_id,
		[
			reset,
			gamemode_0,
			gamemode_1,
			gamemode_2,
			fly
		](Player & player)
		{
			if (reset == GLFW_PRESS)
				player.transform.position = glm::dvec3(0.0, 220.0, 0.0);

			if (gamemode_0 == GLFW_PRESS)
			{
				player.gameMode = Player::GameMode::SURVIVAL;
				player.flying = false;
				player.startFall();
			}
			if (gamemode_1 == GLFW_PRESS)
			{
				player.gameMode = Player::GameMode::CREATIVE;
				player.startFall();
			}
			if (gamemode_2 == GLFW_PRESS)
			{
				player.gameMode = Player::GameMode::SPECTATOR;
			}

			if (fly == GLFW_PRESS)
			{
				if (player.flying)
				{
					player.startFall();
				}
				player.flying = !player.flying;
			}
		}
	);

	m_window.input().getCursorPos(m_mouse_x, m_mouse_y);
}

void UpdateThread::movePlayer()
{
	glm::dvec2 look = glm::dvec2(m_mouse_x - m_last_mouse_x, m_mouse_y - m_last_mouse_y) * m_settings.mouseSensitivity();
	m_last_mouse_x = m_mouse_x;
	m_last_mouse_y = m_mouse_y;

	if (!m_window.input().isCursorCaptured())
	{
		look = glm::dvec2(0.0);
	}

	if (m_current_time - m_last_target_block_update_time > m_target_block_update_interval)
	{
		m_world.updatePlayerTargetBlock(m_world.m_my_player_id);
		m_last_target_block_update_time = m_current_time;
	}

	auto [position, displacement]  = m_world.calculatePlayerMovement(
		m_world.m_my_player_id,
		m_move_forward,
		m_move_backward,
		m_move_left,
		m_move_right,
		m_jump,
		m_sneak,
		static_cast<double>(m_delta_time.count()) / 1e9
	);

	// m_world.applyPlayerMovement(m_world.m_my_player_id, displacement);
	auto packet = std::make_shared<PlayerMovePacket>(m_world.m_my_player_id, position, displacement);
	m_client.sendPacket(packet);

	m_world.updatePlayer(
		m_world.m_my_player_id,
		look.x,
		look.y
	);

	m_world_scene.camera() = m_world.getCamera(m_world.m_my_player_id);
}

void UpdateThread::handlePackets()
{
	int i = 0;
	while (i < 10 && m_client.getQueueSize())
	{
		auto packet = m_client.popPacket();
		m_packet_handler.handlePacket(packet);
	}
}
