#pragma once 

#include <array>

template <typename T, std::size_t size>
class RLE_TEST
{
public:
	typedef std::array<T, size> DataType;
	typedef std::pair<T, uint32_t> RLEPair;
	RLE_TEST()
	{

	};
	RLE_TEST(const DataType & data)
	{
		setData(data);
	};

	static size_t sizeOfPair()
	{
		return sizeof(RLEPair);
	}

	void setData(const DataType & data)
	{
		m_data.clear();
		m_data.push_back(std::make_pair(data[0], 1));
		for(size_t i = 1; i < size; ++i)
		{
			if(data[i] == m_data.back().first)
				m_data.back().second++;
			else
				m_data.push_back(std::make_pair(data[i], 1));
		}
	}

	std::array<T, size> getData() const
	{
		std::array<T, size> ret;
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
