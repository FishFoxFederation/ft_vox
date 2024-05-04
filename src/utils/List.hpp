#pragma once

#include <unordered_map>
#include <mutex>
#include <vector>

template <typename IdType>
class IdGenerator
{

public:

	IdGenerator() : m_next_id(1) {}
	~IdGenerator() {}

	IdGenerator(IdGenerator & other) = delete;
	IdGenerator(IdGenerator && other) = delete;
	IdGenerator & operator=(IdGenerator & other) = delete;
	IdGenerator & operator=(IdGenerator && other) = delete;

	IdType nextId()
	{
		return m_next_id++;
	}

	static const inline IdType invalid_id = 0;

private:

	IdType m_next_id;

};

template <typename Key, typename Value, typename container = std::unordered_map<Key, Value>, typename IdGen = IdGenerator<Key>>
class IdList: private container
{

public:

	IdList() {}
	~IdList() {}

	IdList(const IdList & other) = delete;
	IdList(IdList && other) = delete;
	IdList & operator=(const IdList & other) = delete;
	IdList & operator=(IdList && other) = delete;


	std::lock_guard<std::mutex> lock()
	{
		return std::lock_guard<std::mutex>(m_mutex);
	}

	using container::begin;
	using container::end;
	using container::at;
	using container::find;

	Value get(const Key & key)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return container::at(key);
	}

	uint32_t size() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return container::size();
	}

	bool contains(const Key & key) const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return container::find(key) != container::end();
	}

	Key insert(const Value & value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		Key key = m_id_generator.nextId();
		container::insert(std::make_pair(key, value));
		return key;
	}

	Key insert(Value && value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		Key key = m_id_generator.nextId();
		container::insert(std::make_pair(key, std::move(value)));
		return key;
	}

	void insert(const Key & key, const Value & value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		container::insert(std::make_pair(key, value));
	}

	void insert(const Key & key, Value && value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		container::insert(std::make_pair(key, std::move(value)));
	}

	void erase(const Key & key)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		container::erase(key);
	}

	std::vector<Value> values() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::vector<Value> values;
		values.reserve(container::size());
		for (const auto & [key, value] : *this)
		{
			values.push_back(value);
		}
		return values;
	}

	static const inline Key invalid_id = IdGen::invalid_id;

private:

	IdGen m_id_generator;

	mutable std::mutex m_mutex;
};

