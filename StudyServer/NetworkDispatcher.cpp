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

void NetworkDispatcher::PushPacket(std::unique_ptr<NetworkPacket> packet)
{
	mPacketQueue.push(std::move(packet));
}


void NetworkDispatcher::_DispatchThread()
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

		printf_s("Received Body: %s\n", packet->Body.data());
		mService->ProcessPacket(std::move(packet));
	}
}

//void NetworkDispatcher::SendThread()
//{
//	while (mIsRunning)
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(1));
//
//		if (mSendQueue.empty())
//		{
//			continue;
//		}
//
//		std::shared_ptr<NetworkContext> context = nullptr;
//
//		if (false == mSendQueue.try_pop(context))
//		{
//			continue;
//		}
//
//		if (context->mSessionID < 0 || context->mSessionID >= mSessionList.size())
//		{
//			printf_s("SendThread: Invalid SessionID: %d\n", context->mSessionID);
//			continue;
//		}
//
//		auto client = mSessionList.at(context->mSessionID).lock();
//
//		if (false == client->IsConnected())
//		{
//			printf_s("SendThread: Client Disconnected: %d\n", context->mSessionID);
//			continue;
//		}
//
//		if (false == client->Send(*context))
//		{
//			printf_s("SendThread: Send Failed: %d\n", context->mSessionID);
//			continue;
//		}
//
//		printf_s("SendThread: Send Success: %d\n", context->mSessionID);
//	}
//}
