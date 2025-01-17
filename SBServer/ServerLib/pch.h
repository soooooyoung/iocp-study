#pragma once

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#define WIN32_LEAN_AND_MEAN     
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h> 

template<class T> void SAFE_DELETE(T*& p) { if (p) delete p; p = nullptr; }

#define SPDLOG_HEADER_ONLY