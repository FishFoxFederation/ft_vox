#pragma once

#include "PlayerMovePacket.hpp"
#include "PlayerConnectedPacket.hpp"
#include "ConnectionPacket.hpp"
#include "DisconnectPacket.hpp"
#include "BlockActionPacket.hpp"
#include "PingPacket.hpp"
#include "PlayerListPacket.hpp"
#include "ChunkPacket.hpp"


/**
 * C > S : ConnectionPacket
 * S > all C : PlayerConnectedPacket
 * 
 * C > S : PlayerMovePacket
 * S > all C : PlayerMovePacket
 * 
 */
