#include "pch.h"
#include "NetworkDispatcher.h"
#include "NetworkContext.h"
#include "Service.h"

NetworkDispatcher::NetworkDispatcher() : mIsRunning(false)
{
}

NetworkDispatcher::~NetworkDispatcher()
{
	mIsRunning = false;
	mService = nullptr;

	if (mDispatchThread.joinable())
	{
		mDispatchThread.join();
	}
}

bool NetworkDispatcher::Initialize(std::unique_ptr<Service> service, int nRemainThread)
{
	if (mService != nullptr)
	{
		return false;
	}

	mDispatchThread = std::thread(&NetworkDispatcher::_DispatchThread, this);
	mService = std::move(service);
	mIsRunning = true;

	return true;
}

void NetworkDispatcher::PushPacket(MemoryPool<Packet>::UniquePtr packet)
{
	std::lock_guard<std::mutex> lock(mMutex);
	mPacketQueue.push(std::move(packet));
}


void NetworkDispatcher::_DispatchThread()
{
	while (mIsRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		std::lock_guard<std::mutex> lock(mMutex);

		if (mPacketQueue.empty())
		{
			continue;
		}

		mService->ProcessPacket(*(mPacketQueue.front()));

		mPacketQueue.pop();
	}
}
