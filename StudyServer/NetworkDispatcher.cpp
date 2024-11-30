#include "pch.h"
#include "NetworkDispatcher.h"
#include "NetworkClient.h"
#include "NetworkPacket.h"
#include "NetworkContext.h"

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

bool NetworkDispatcher::Initialize(int nRemainThread)
{
	mIsRunning = true;
	mDispatchThread = std::thread(&NetworkDispatcher::DispatchThread, this);

	for (int i = 0; i < nRemainThread; ++i)
	{
		mSendThreadPool.emplace_back([this]() { SendThread(); });

	}
	return true;
}

void NetworkDispatcher::AddSession(std::weak_ptr<NetworkClient> session)
{
	mSessionList.push_back(session);
}

void NetworkDispatcher::PushPacket(std::unique_ptr<NetworkPacket> packet)
{
	mPacketQueue.push(std::move(packet));
}

void NetworkDispatcher::PushSend(int sessionID, void* data, int size)
{
	std::shared_ptr<NetworkContext> context = std::make_shared<NetworkContext>();
	
	if (false == context->Write(data, size))
	{
		printf_s("PushSend: Write Failed for SessionID: %d\n", sessionID);
		return;
	}

	mSendQueue.push(context);
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


		printf_s("Received Body: %s\n", packet->Body.data());

		// TODO: Dispatch Packet
	}
}

void NetworkDispatcher::SendThread()
{
	while (mIsRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (mSendQueue.empty())
		{
			continue;
		}

		std::shared_ptr<NetworkContext> context = nullptr;

		if (false == mSendQueue.try_pop(context))
		{
			continue;
		}

		if (context->mSessionID < 0 || context->mSessionID >= mSessionList.size())
		{
			printf_s("SendThread: Invalid SessionID: %d\n", context->mSessionID);
			continue;
		}

		auto client = mSessionList.at(context->mSessionID).lock();

		if (false == client->IsConnected())
		{
			printf_s("SendThread: Client Disconnected: %d\n", context->mSessionID);
			continue;
		}

		if (false == client->Send(*context))
		{
			printf_s("SendThread: Send Failed: %d\n", context->mSessionID);
			continue;
		}

		printf_s("SendThread: Send Success: %d\n", context->mSessionID);
	}
}
