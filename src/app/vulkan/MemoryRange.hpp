#pragma once

#include <cstdint>
#include <map>

class MemoryRange
{

public:

	~MemoryRange() {}

	MemoryRange(const uint64_t & capacity = 0):
		m_capacity(capacity)
	{
		add(capacity);
	}

	uint64_t capacity() { return m_capacity; }

	void add(const uint64_t & capacity)
	{
		m_capacity += capacity;
	}

	uint64_t alloc(const uint64_t & alloc_size)
	{
		uint64_t address = 0;

		for (auto it = m_used_ranges.begin(); it != m_used_ranges.end(); ++it)
		{
			if (it->first - address >= alloc_size)
			{
				m_used_ranges[address] = alloc_size;
				return address;
			}
			address = it->first + it->second;
		}

		if (m_used_ranges.empty() && m_capacity >= alloc_size)
		{
			m_used_ranges[0] = alloc_size;
			return 0;
		}

		return capacity();
	}

	void free(const uint64_t & address)
	{
		m_used_ranges.erase(address);
	}

private:

	std::map<uint64_t, uint64_t> m_used_ranges;
	uint64_t m_capacity;

};

// add 10
// 0 10

// alloc 5
// 0 0 5 10

// add 10
// 0 0 5 20

// alloc 10
// 0 0 5 5 15 20

// free 0
// 0 5 15 20