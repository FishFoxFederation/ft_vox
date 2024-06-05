#include "Mob.hpp"

Mob::Mob()
{
}

Mob::~Mob()
{
}

bool Mob::canJump() const
{
	return on_ground && jump_remaining > 0;
}

void Mob::startJump()
{
	jump_remaining--;
	jumping = true;
}

void Mob::startFall()
{
	fall_start_time = std::chrono::steady_clock::now();
	fall_start_position = transform.position;
}

double Mob::fallDuration()
{
	return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - fall_start_time).count();
}
