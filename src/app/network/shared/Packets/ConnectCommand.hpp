#pragma once

#include "Command.hpp"

class ConnectCommand : public Command
{
public:
	#define CONNECT_COMMAND_SIZE 0
	ConnectCommand();
	~ConnectCommand();

	ConnectCommand(const ConnectCommand& other) = delete;
	ConnectCommand& operator=(const ConnectCommand& other) = delete;

	ConnectCommand(ConnectCommand&& other) = delete;
	ConnectCommand& operator=(ConnectCommand&& other) = delete;
private:
};
