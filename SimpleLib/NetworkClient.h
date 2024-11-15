﻿#pragma once

class NetworkContext;
class NetworkClient : public PoolObject<NetworkClient>
{
public:
	virtual ~NetworkClient();

	UINT32 GetSessionID() const { return mSessionID; }
	SOCKET GetSocket() const { return mSocket; }
	UINT64 GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }

	void SetSessionID(UINT32 sessionID) { mSessionID = sessionID; }

	virtual bool Init();
	virtual void Reset();

	void Close(bool bIsForce = false);
	void OnConnect(SOCKET& socket);

	void PushSend(std::uint8_t* pData, size_t size);
protected:
	UINT32 mSessionID = 0;
	SOCKET mSocket = INVALID_SOCKET;
	UINT64 mLatestClosedTimeSec = 0;

	SecureQueue<NetworkContext*> mSendQueue;
};

