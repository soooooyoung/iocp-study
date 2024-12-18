#pragma once

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#define WIN32_LEAN_AND_MEAN

// Windows
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <Windows.h>

// STL
#include <memory>
#include <vector>
#include <thread>
#include <array>
#include <span>
#include <string>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <functional>

#include "SharedEnum.h"
#include "SharedConst.h"
#include "NetworkPacket.h"
#include "MemoryPool.h"