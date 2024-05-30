#pragma once 

#include "IPacket.hpp"

// struct HandleArgs
// {
// 	union {
// 		Server * server;
// 		Client * client;
// 	};
// 	enum class Env { SERVER, CLIENT };
// 	Env env;
// 	World * world;
// };

class PacketHandler
{
public:
	void HandlePacket(std::shared_ptr<IPacket> packet);
private:
	// all the handlers

};
