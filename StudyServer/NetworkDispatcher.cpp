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
	//if (mPacketThread.joinable())
	//{
	//	mPacketThread.join();
	//}
}

bool NetworkDispatcher::Initialize()
{
	mIsRunning = true;
	mDispatchThread = std::thread(&NetworkDispatcher::DispatchThread, this);
	//mPacketThread = std::thread(&NetworkDispatcher::PacketThread, this);
	return true;
}

void NetworkDispatcher::PushPacket(std::shared_ptr<NetworkPacket> packet)
{
	mPacketQueue.push(std::move(packet));
}

//void NetworkDispatcher::EnqueueClientPacket(std::weak_ptr<NetworkContext> context)
//{
//	mIncomingPacketQueue.push(context);
//}

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

//void NetworkDispatcher::PacketThread()
//{
//	while (mIsRunning)
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(1));
//
//		if (mIncomingPacketQueue.empty())
//		{
//			continue;
//		}
//
//		std::weak_ptr<NetworkContext> contextLock;
//		if (false == mIncomingPacketQueue.try_pop(contextLock))
//		{
//			continue;
//		}
//
//		auto context = contextLock.lock();
//
//		if (nullptr == context)
//		{
//			continue;
//		}
//
//		auto dataSize = context->GetDataSize();
//
//
//		// TODO: Make sure to get the entire packet
//		
//		auto packet = std::make_shared<NetworkPacket>();
//
//		// TODO: Deserialize packet
//
//		PushPacket(std::move(packet));
//
//		context->Read(dataSize);
//	}
//}
