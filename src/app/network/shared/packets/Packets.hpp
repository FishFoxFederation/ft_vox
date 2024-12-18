#pragma once

#include "PlayerMovePacket.hpp"
#include "PlayerConnectedPacket.hpp"
#include "ConnectionPacket.hpp"
#include "DisconnectPacket.hpp"
#include "BlockActionPacket.hpp"
#include "PingPacket.hpp"
#include "PlayerListPacket.hpp"
#include "ChunkPacket.hpp"
#include "ChunkRequestPacket.hpp"
#include "ChunkUnloadPacket.hpp"
#include "LoadDistancePacket.hpp"


/**
 * C > S : ConnectionPacket
 * S > all C : PlayerConnectedPacket
 * 
 * C > S : PlayerMovePacket
 * S > all C : PlayerMovePacket
 * 
 */
