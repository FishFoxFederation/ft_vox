#include "ClientWorld.hpp"

/*************************************
 *  MOBS
 *************************************/

void ClientWorld::createBaseMob(const glm::dvec3 & position, uint64_t player_id)
{
	auto [succes, entity] = m_ecs_manager.createEntity();

	if (!succes)
	{
		LOG_ERROR("Failed to create mob entity");
		return;
	}
	m_render_api.addEntity(entity, {m_render_api.getCubeMeshId(), glm::translate(glm::dmat4(1.0), position)});

	m_ecs_manager.add(entity,
		Position{position},
		Velocity{glm::vec3(0.0f)},
		Acceleration{glm::vec3(10.0f, 10.0f, 10.0f)},
		AITarget{player_id},
		Mesh{0}
	);

	LOG_INFO("Created mob entity: " << entity);
}

void ClientWorld::InputSystem()
{
	auto & input_singleton = m_ecs_manager.getSingleton<input>();

	const Input::KeyState move_forward_key_status = m_window.input().getKeyState(GLFW_KEY_W);
	const Input::KeyState move_left_key_status = m_window.input().getKeyState(GLFW_KEY_A);
	const Input::KeyState move_backward_key_status = m_window.input().getKeyState(GLFW_KEY_S);
	const Input::KeyState move_right_key_status = m_window.input().getKeyState(GLFW_KEY_D);
	const Input::KeyState jump_key_status = m_window.input().getKeyState(GLFW_KEY_SPACE);
	const Input::KeyState sneak_key_status = m_window.input().getKeyState(GLFW_KEY_LEFT_SHIFT);
	const Input::KeyState attack_key_status = m_window.input().getMouseButtonState(GLFW_MOUSE_BUTTON_LEFT);
	const Input::KeyState use_key_status = m_window.input().getMouseButtonState(GLFW_MOUSE_BUTTON_RIGHT);

	if (move_forward_key_status == Input::KeyState::PRESSED) {
		input_singleton.move_forward = 1;
	}
	else if (move_forward_key_status == Input::KeyState::RELEASED) input_singleton.move_forward = 0;

	if (move_left_key_status == Input::KeyState::PRESSED) input_singleton.move_left = 1;
	else if (move_left_key_status == Input::KeyState::RELEASED) input_singleton.move_left = 0;

	if (move_backward_key_status == Input::KeyState::PRESSED) input_singleton.move_backward = 1;
	else if (move_backward_key_status == Input::KeyState::RELEASED) input_singleton.move_backward = 0;

	if (move_right_key_status == Input::KeyState::PRESSED) input_singleton.move_right = 1;
	else if (move_right_key_status == Input::KeyState::RELEASED) input_singleton.move_right = 0;

	if (jump_key_status == Input::KeyState::PRESSED) input_singleton.jump = 1;
	else if (jump_key_status == Input::KeyState::RELEASED) input_singleton.jump = 0;

	if (sneak_key_status == Input::KeyState::PRESSED) input_singleton.sneak = 1;
	else if (sneak_key_status == Input::KeyState::RELEASED) input_singleton.sneak = 0;

	if (attack_key_status == Input::KeyState::PRESSED) input_singleton.attack = 1;
	else if (attack_key_status == Input::KeyState::RELEASED) input_singleton.attack = 0;

	if (use_key_status == Input::KeyState::PRESSED) input_singleton.use = 1;
	else if (use_key_status == Input::KeyState::RELEASED) input_singleton.use = 0;

	m_window.input().getCursorPos(input_singleton.mouse_x, input_singleton.mouse_y);
}

void ClientWorld::MovementSystem()
{
	auto view = m_ecs_manager.view<Position, Velocity>();
	auto delta_time_second = m_ecs_manager.getSingleton<Time>().delta_time_ms / 1000.0;


	for ( auto entity : view )
	{
		auto [ position, velocity ] = m_ecs_manager.getComponents<Position, Velocity>(entity);

		position.p += velocity.v * delta_time_second;
	}
}

void ClientWorld::renderSystem()
{
	auto view = m_ecs_manager.view<Position, Mesh>();
	for (auto entity : view )
	{
		auto [ position, mesh ] = m_ecs_manager.getComponents<Position, Mesh>(entity);

		m_render_api.updateEntity(entity, [position](MeshRenderData & data)
		{
			data.model = glm::translate(glm::dmat4(1.0), position.p);
		});
	}
}

void ClientWorld::AISystem()
{
	auto view = m_ecs_manager.view<Position, AITarget, Velocity, Acceleration>();

	for (auto entity : view)
	{
		auto [ position, ai_target, velocity, acceleration ] = m_ecs_manager.getComponents<Position, AITarget, Velocity, Acceleration>(entity);

		auto player_position = getPlayerPosition(ai_target.target_id);

		glm::dvec3 diff = player_position - position.p;
		if (glm::length(diff) >= 0.0001)
			diff = glm::normalize(diff);

		velocity.v = diff * acceleration.a;
	}
}

void ClientWorld::updateSystems()
{
	auto static last_move = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	// LOG_INFO("Updating systems");
	timeSystem();
	InputSystem();
	AISystem();
	if (now - last_move > std::chrono::milliseconds(20))
	{
		playerMoveSystem();
		last_move = now;
	}
	MovementSystem();
	renderSystem();
}

void ClientWorld::timeSystem()
{
	static auto last = std::chrono::steady_clock::now();

	auto singleton = m_ecs_manager.getSingleton<Time>();
	auto now = std::chrono::steady_clock::now();

	auto delta = now - last;
	last = now;

	singleton.delta_time_ms = delta.count() / 1e6; 
	singleton.now = now.time_since_epoch();
}

void ClientWorld::playerMoveSystem()
{
	static bool first = true;
	auto & inputs = m_ecs_manager.getSingleton<input>();
	auto & infos = m_ecs_manager.getSingleton<MoveInfo>();
	auto time = m_ecs_manager.getSingleton<Time>();

	if (first) {
		infos.last_move = time.now;
		first = false;
	}

	double delta_time_second = std::chrono::duration_cast<std::chrono::milliseconds>(time.now - infos.last_move).count() / 1000.0;

	glm::dvec2 look = glm::dvec2(inputs.mouse_x - inputs.last_mouse_x, inputs.mouse_y - inputs.last_mouse_y) * 0.2;
	inputs.last_mouse_x = inputs.mouse_x;
	inputs.last_mouse_y = inputs.mouse_y;

	//speed limiter
	if (delta_time_second > 0.2) delta_time_second = 0.2;


	if (!m_window.input().isCursorCaptured())
	{
		look = glm::dvec2(0.0);
	}

	auto [position, displacement]  = calculatePlayerMovement(
		m_my_player_id,
		inputs.move_forward,
		inputs.move_backward,
		inputs.move_left,
		inputs.move_right,
		inputs.jump,
		inputs.sneak,
		delta_time_second
	);

	applyPlayerMovement(m_my_player_id, displacement);
	if (glm::length(displacement) > 0.01)
	{
		// glm::vec3 new_position = position + displacement;
		auto packet = std::make_shared<PlayerMovePacket>(m_my_player_id, position, displacement);
		m_client.send({packet, 0});
	} 

	updatePlayerCamera(
		m_my_player_id,
		look.x,
		look.y
	);

	m_render_api.setCamera(getCamera(m_my_player_id));
	infos.last_move = time.now;
}
