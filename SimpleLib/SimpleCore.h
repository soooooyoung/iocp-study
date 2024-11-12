#pragma once
#define WIN32_LEAN_AND_MEAN

// Windows
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
#include <basetsd.h>

// STL
#include <ranges>
#include <array>
#include <memory>
#include <mutex>
#include <queue>
#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <functional>

// SimpleLib
#include "SecureQueue.h"
#include "StaticPool.h"
#include "Singleton.h"
#include "NetworkPacket.h"