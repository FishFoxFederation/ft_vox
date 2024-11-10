#pragma once

#include "ThreadSafeQueue.hpp"
#include "IPacket.hpp"
#include <memory>

typedef ThreadSafeQueue<std::shared_ptr<IPacket>> ThreadSafePacketQueue;
