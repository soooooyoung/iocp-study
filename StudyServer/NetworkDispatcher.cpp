#include "pch.h"
#include "NetworkDispatcher.h"
#include "NetworkContext.h"
#include "NetworkPacket.h"

NetworkDispatcher::NetworkDispatcher()
{
}

NetworkDispatcher::~NetworkDispatcher()
{
	mIsRunning = false;
	if (mDispatchThread.joinable())
	{
		mDispatchThread.join();
	}
}

bool NetworkDispatcher::Initialize()
{
	mIsRunning = true;
	mDispatchThread = std::thread(&NetworkDispatcher::DispatchThread, this);
	return true;
}

void NetworkDispatcher::PushPacket(std::unique_ptr<NetworkPacket> packet)
{
	mPacketQueue.push(std::move(packet));
}

void NetworkDispatcher::DispatchThread()
{
	while (mIsRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (mPacketQueue.empty())
		{
			continue;
		}

		std::shared_ptr<NetworkPacket> packet;

		if (false == mPacketQueue.try_pop(packet))
		{
			continue;
		}



		// TODO: Dispatch Packet
	}
}
