#pragma once

class NetworkContext;
class NetworkClient : public PoolObject<NetworkClient>
{
public:
	virtual ~NetworkClient();

	UINT32 GetSessionID() const { return mSessionID; }
	SOCKET GetSocket() const { return mSocket; }
	UINT64 GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }

	void SetSessionID(UINT32 sessionID) { mSessionID = sessionID; }
	void SetSocket(SOCKET socket) { mSocket = socket; }

	virtual bool Init();
	virtual void Reset();

	void Close(bool bIsForce = false);
protected:
	UINT32 mSessionID = 0;
	SOCKET mSocket = INVALID_SOCKET;
	UINT64 mLatestClosedTimeSec = 0;

	SecureQueue<std::shared_ptr<NetworkContext>> mSendQueue;
};

