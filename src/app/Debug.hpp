#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <any>

template <typename Value>
class Debug
{

public:

	static void set(const std::string & key, const Value & value)
	{
		std::lock_guard<std::mutex> lock(m_debug_data_mutex);
		m_debug_data[key] = value;
	}

	static Value get(const std::string & key)
	{
		std::lock_guard<std::mutex> lock(m_debug_data_mutex);
		return m_debug_data.at(key);
	}

private:

	static inline std::unordered_map<std::string, Value> m_debug_data;
	static inline std::mutex m_debug_data_mutex;

	Debug() = delete;
	~Debug() = delete;
	Debug(const Debug &) = delete;
	Debug & operator=(const Debug &) = delete;
	Debug(Debug &&) = delete;
	Debug & operator=(Debug &&) = delete;
};

