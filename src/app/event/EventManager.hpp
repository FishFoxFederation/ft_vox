#pragma once

#include "AbstractEvent.hpp"
#include "Events.hpp"

#include <string>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <mutex>

namespace Event
{
	template<typename EventType>
	using Handler = std::function<void(const EventType & e)>;

	class HandlerWrapperInterface
	{

	public:

		void exec(const AbstractEvent & e) { call(e); }

		virtual std::string getType() const = 0;

	private:

		virtual void call(const AbstractEvent& e) = 0;

	};


	template<typename EventType>
	class HandlerWrapper : public HandlerWrapperInterface
	{

	public:

		explicit HandlerWrapper(const Handler<EventType> & handler):
			m_handler(handler),
			m_handlerType(m_handler.target_type().name())
		{
		}

	private:

		void call(const AbstractEvent & e) override
		{
			if (e.getType() == EventType::getStaticType())
			{
				m_handler(static_cast<const EventType &>(e));
			}
		}

		std::string getType() const override { return m_handlerType; }

		Handler<EventType> m_handler;
		const std::string m_handlerType;
	};


	class Manager
	{

	public:

		Manager() = default;
		virtual ~Manager() = default;

		template<typename EventType>
		void subscribe(const Handler<EventType> & handler)
		{
			std::unique_lock lock(m_mutex);

			auto wrapper = std::make_unique<HandlerWrapper<EventType>>(handler);
			m_subscribers[EventType::getStaticType()].push_back(std::move(wrapper));
		}

		template<typename EventType>
		void unsubscribe(const Handler<EventType> & handler)
		{
			std::unique_lock lock(m_mutex);

			const Type type = EventType::getStaticType();
			const auto it = m_subscribers.find(type);
			if (it != m_subscribers.end())
			{
				auto & subscribers = it->second;
				const auto it = std::find_if(subscribers.begin(), subscribers.end(),
					[&handler](const std::unique_ptr<HandlerWrapperInterface> & wrapper)
					{
						return wrapper->getType() == handler.target_type().name();
					}
				);
				if (it != subscribers.end())
				{
					subscribers.erase(it);
				}
			}
		}

		void triggerEvent(const AbstractEvent & e)
		{
			std::shared_lock lock(m_mutex);

			const Type type = e.getType();
			const auto it = m_subscribers.find(type);
			if (it != m_subscribers.end())
			{
				for (auto & subscriber : it->second)
				{
					subscriber->exec(e);
				}
			}
		}

	private:

		std::unordered_map<Type, std::vector<std::unique_ptr<HandlerWrapperInterface>>> m_subscribers;
		std::shared_mutex m_mutex;

	};

} // namespace Event
