#pragma once 

#include <array>

template <typename T>
class RLE_TEST
{
public:
	typedef std::pair<T, uint16_t> RLEPair;

	RLE_TEST()
	{

	};

	static size_t sizeOfPair()
	{
		return sizeof(RLEPair);
	}

	void compressData(const T * data, const size_t & size)
	{
		m_data.clear();
		m_data.push_back(std::make_pair(data[0], 1));
		for(size_t i = 1; i < size; ++i)
		{
			if(data[i] == m_data.back().first && m_data.back().second < std::numeric_limits<uint16_t>::max())
				m_data.back().second++;
			else
				m_data.push_back(std::make_pair(data[i], 1));
		}
	}

	void setContent(const void * data, const size_t & size)
	{
		m_data.clear();
		m_data.insert(m_data.begin(), (RLEPair*)data, (RLEPair*)data + size / sizeof(RLEPair));
	}

	std::vector<T> getData() const
	{
		std::vector<T> ret;
		for (auto [value, count] : m_data)
		{
			for(size_t j = 0; j < count; ++j)
				ret.push_back(value);
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

	/**
	 * @brief Get the size in bytes of the underlying compressed data
	 * 
	 * @return std::size_t 
	 */
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
