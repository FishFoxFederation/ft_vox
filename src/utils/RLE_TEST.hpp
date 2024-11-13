#pragma once 

#include <array>

template <typename T, size_t N>
class RLE_TEST
{
public:
	typedef T value_type;
	typedef std::pair<value_type, uint32_t> RLEPair;
	typedef std::array<T, N> array_type;
	RLE_TEST()
	{

	};
	RLE_TEST(const array_type & data)
	{
		setData(data);
	};

	static size_t sizeOfPair()
	{
		return sizeof(RLEPair);
	}

	void setData(const array_type & data)
	{
		m_data.clear();
		m_data.push_back(std::make_pair(data[0], 1));
		for(size_t i = 1; i < N; ++i)
		{
			if(data[i] == m_data.back().first && m_data.back().second < std::numeric_limits<uint32_t>::max())
				m_data.back().second++;
			else
				m_data.push_back(std::make_pair(data[i], 1));
		}
	}

	array_type getData() const
	{
		array_type ret;
		size_t i = 0;
		for (auto [value, count] : m_data)
		{
			for (size_t j = 0; j < count; ++j)
			{
				ret[i] = value;
				++i;
			}
		}

		return ret;
	}

	void resize(size_t new_raw_size)
	{
		m_data.resize(new_raw_size / sizeof(RLEPair));
	}

	std::size_t getSize() const
	{
		return m_data.size();
	}

	std::size_t getRawSize() const
	{
		return m_data.size() * sizeof(RLEPair);
	}	

	const std::vector<RLEPair> & getRaw() const
	{
		return m_data;
	}

	std::vector<RLEPair> & getRaw()
	{
		return m_data;
	}
private:
	std::vector<RLEPair> m_data;
};
