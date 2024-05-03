#pragma once 

#include <cstdint>

class Command
{
public:
	~Command();
	
	enum class type : uint8_t
	{
		CONNECT = 0x00
	};

	type getType() const;

protected:
	Command(type commandType);
private:
	Command();
	type m_type;
};
