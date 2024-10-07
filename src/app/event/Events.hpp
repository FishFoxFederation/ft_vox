#pragma once

#include "AbstractEvent.hpp"
#include "Block.hpp"

#include <glm/glm.hpp>

namespace Event
{

	class PlayerMoving : public AbstractEvent
	{

	public:

		PlayerMoving(
			const uint64_t player_id,
			const glm::dvec3 & old_pos,
			const glm::dvec3 & displacement,
			const BlockID & block
		):
			player_id(player_id),
			old_position(old_pos),
			displacement(displacement),
			ground_block(block)
		{}

		~PlayerMoving() = default;

		Type getType() const noexcept override { return getStaticType(); }
		static Type getStaticType() { return typeid(PlayerMoving); }

		uint64_t player_id;
		glm::dvec3 old_position;
		glm::dvec3 displacement;
		BlockID ground_block;

	};

} // namespace Event
