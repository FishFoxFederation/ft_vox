#pragma once

#include <unordered_map>

template <typename IdType>
class IdGenerator
{

public:

	IdGenerator() : m_next_id(0) {}
	~IdGenerator() {}

	IdGenerator(IdGenerator & other) = delete;
	IdGenerator(IdGenerator && other) = delete;
	IdGenerator & operator=(IdGenerator & other) = delete;
	IdGenerator & operator=(IdGenerator && other) = delete;

	IdType nextId()
	{
		return m_next_id++;
	}

private:

	IdType m_next_id;

};

template <typename Key, typename Value, typename container = std::unordered_map<Key, Value>, typename IdGen = IdGenerator<Key>>
class IdList: private container
{

public:

	IdList() {}
	~IdList() {}

	IdList(IdList & other) = delete;
	IdList(IdList && other) = delete;
	IdList & operator=(IdList & other) = delete;
	IdList & operator=(IdList && other) = delete;

	using container::contains;
	using container::find;
	using container::at;

	using container::begin;
	using container::end;

	using container::size;
	using container::empty;

	Key insert(const Value & value)
	{
		Key key = m_id_generator.nextId();
		container::insert(std::make_pair(key, value));
		return key;
	}

	Key insert(Value && value)
	{
		Key key = m_id_generator.nextId();
		container::insert(std::make_pair(key, std::move(value)));
		return key;
	}

private:

	IdGen m_id_generator;
};

