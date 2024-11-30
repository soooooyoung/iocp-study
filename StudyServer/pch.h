#pragma once

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#define WIN32_LEAN_AND_MEAN

// Windows
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>

// STL
#include <memory>
#include <vector>
#include <thread>
#include <array>
#include <span>
#include <string>
#include <cstdint>
#include <queue>

#include "SharedEnum.h"
#include "SharedConst.h"
#include "NetworkPacket.h"