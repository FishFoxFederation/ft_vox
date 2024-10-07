#pragma once

#include <string>
#include <typeindex>

namespace Event
{
	typedef std::type_index Type;

	class AbstractEvent
	{

	public:

		AbstractEvent() = default;
		virtual ~AbstractEvent() = default;

		virtual Type getType() const noexcept = 0;
		static Type getStaticType() { return typeid(AbstractEvent); }

	};

} // namespace Event
