#pragma once

#include "define.hpp"
#include "AThreadWrapper.hpp"
#include "window.hpp"
#include "WorldScene.hpp"

class UpdateThread : public AThreadWrapper
{
public:

	UpdateThread(
		Window & window,
		WorldScene & worldScene
	);
	~UpdateThread();

	UpdateThread(UpdateThread& other) = delete;
	UpdateThread(UpdateThread&& other) = delete;
	UpdateThread& operator=(UpdateThread& other) = delete;
	UpdateThread& operator=(UpdateThread&& other) = delete;

private:

	Window & m_window;

	WorldScene & m_world_scene;

	/**
	 * @brief WIP
	 */
	void init() override;

	/**
	 * @brief WIP
	 */
	void loop() override;
};
