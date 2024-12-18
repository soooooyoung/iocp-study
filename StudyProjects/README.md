# IOCP Study

[![en](https://img.shields.io/badge/lang-english-yellow.svg)](README.md)
[![kr](https://img.shields.io/badge/lang-한국어-red.svg)](README.kr.md)

## Tutorial Branches

These branches follow steps [IOCP Tutorial Repo](https://github.com/jacking75/edu_cpp_IOCP.git) by [jacking75](https://github.com/jacking75) for learning purposes

| Step                                                                  | Description                                            |
| :-------------------------------------------------------------------- | :----------------------------------------------------- |
| [Step 1-2](https://github.com/soooooyoung/iocp-study/tree/IOCP_01_02) | Create Echoserver utilizing IOCP                       |
| [Step 3](https://github.com/soooooyoung/iocp-study/tree/IOCP_03)      | Separate application logic from network handling logic |
| [Step 4](https://github.com/soooooyoung/iocp-study/tree/IOCP_04)      | Separate operations in individual threads              |
| [Step 5](https://github.com/soooooyoung/iocp-study/tree/IOCP_05)      | Implement **\*1-Send** by buffer accumulation          |
| [Step 6](https://github.com/soooooyoung/iocp-study/tree/IOCP_06)      | Implement queue-based **\*1-Send**                     |
| [Step 7](https://github.com/soooooyoung/iocp-study/tree/IOCP_07)      | Use asynchronous Accept                                |

**\*1-Send**: Implement logic to wait for the completion of one send operation before initiating the next, ensuring orderly and non-overlapping I/O processing.

## Enhancement Implementation

Process to implement my own server by leveraging concepts and structures learned from the tutorial.

This server will be tailored to specific requirements and use cases beyond the tutorial examples.

This approach will outline:

- Custom architecture decisions.
- Advanced features or optimizations that differ from the tutorial.
- Specific challenges faced and how they were addressed.

### Architecture

#### Network Library

| Component      | Role                                                                                                                                 |
| :------------- | :----------------------------------------------------------------------------------------------------------------------------------- |
| IOCPHandler    | Manages I/O Completion Port (IOCP) operations, including worker threads for processing I/O events and handling client registration.  |
| NetworkClient  | Represents a client connection, maintains session ID and socket, handles send operations.                                            |
| \*ListenClient | Listens for incoming client connections, binds to a port, and handles accepting connections asynchronously.                          |
| NetworkManager | Coordinates overall network operations, manages client and listener registration, sends packets, and interacts with IOCPHandler.     |
| SimpleCore     | Provides foundational includes and utility classes used throughout the network library, such as memory pools and singleton patterns. |
| NetworkContext | Represents the context of each I/O operation, maintaining buffer and connection details.                                             |

\*Inherits **NetworkClient**

#### Server Logic

### Using OVERLAPPED with IOCP and Custom Context

Primary challenge was ensuring the compatibility of custom NetworkContext objects with the Windows I/O Completion Port (IOCP) mechanisms while maintaining application-specific data.

#### Initial Implementation:

In the initial design, the NetworkContext class contained a member variable of type WSAOVERLAPPED:

```cpp
class NetworkContext
{
private:
	std::array<std::uint8_t, 8096> mBuffer;
	int mReadPos = 0;
	int mWritePos = 0;
public:
	WSAOVERLAPPED mWsaOverlapped = { 0, };
//...
}
```

This allowed the association of asynchronous I/O operations with the `WSAOVERLAPPED` structure as a member, but had limitations when additional context information (other than buffer) was required.

Directly casting the entire NetworkContext object to functions like WSASend or WSARecv could lead to issues, as these functions expect a pointer to an OVERLAPPED structure.

#### Revised Implementation:

To address this, the NetworkContext class was modified to inherit from OVERLAPPED:

```cpp
class NetworkContext : public OVERLAPPED
{
private:
	std::array<std::uint8_t, 8096> mBuffer;
	int mReadPos = 0;
	int mWritePos = 0;
//..
}
```

This simplified proper memory alignment, allowing the system to access the OVERLAPPED fields directly. It also eliminated the need for pointer arithmetic to retrieve the rest of the context. However, it is important to note that such inheritance might be considered misuse of inheritance because such relationship doesn't semantically exist between Network Context and OVERLAPPED.

### Handling Packet Processing on IOThread

Deserialization of received packets is performed directly in the IOCP thread. IOCP threads are lightweight and can efficiently handle both IO-bound and CPU-bound tasks.

> **\_HandleReceive**: handles the completion of a network receive operation

```cpp
void NetworkManager::_HandleReceive(NetworkClient& client, NetworkContext& context, int transferred)
{
	// ensure a valid number of bytes
	if (transferred <= 0)
	{
		printf_s("_HandleReceive Error: %d\n", WSAGetLastError());
		return;
	}

	// update buffer information for internal buffer
	if (false == context.Write(transferred))
	{
		printf_s("_HandleReceive Error: Failed to Write\n");
		return;
	}

	// Packet Deserialization
	// converts the raw bytes into a structured NetworkPacket
	std::unique_ptr<NetworkPacket> packet = client.GetPacket();

	if (nullptr == packet)
	{
		printf_s("_HandleReceive Error: Failed to GetPacket\n");
		return;
	}

	packet->Header.SessionID = client.GetSessionID();
	mDispatcher->PushPacket(std::move(packet));

	// Bind for next receive
	client.Receive();
}
```

By processing the data directly on the same IOCP thread, the CPU can leverage cache locality because the data is likely still in the CPU cache from the I/O operation.

This eliminates the need to load the same data into cache again if it were passed to a separate worker thread, reducing memory access latency.

For high-frequency, small packets, this approach improves overall throughput by avoiding the overhead of transferring data to another thread. However, larger packets, which may require more complex processing, could risk overloading the IOCP thread.

Once the packet is fully deserialized, it is forwarded to the NetworkDispatcher for routing to the application layer.

### Packet Queue Management

Packets pushed to NetworkDispatcher are directly processed from packet queue within the critical section. This approach introduced potential performance bottlenecks as the mutex remained locked during packet processing, blocking IOCP threads from accessing mPacketQueue.

To address this, the queue management logic was updated to introduce a secondary work queue that temporarily holds the packets for processing outside the critical section.

#### Initial Implementation:

```cpp
void NetworkDispatcher::_DispatchThread()
{
    while (mIsRunning)
    {
		std::lock_guard<std::mutex> lock(mMutex);

		if (mPacketQueue.empty())
		{
			continue;
		}


		mService->ProcessPacket(*(mPacketQueue.front()));
		mPacketQueue.pop();
    }
}
```

#### Revised Implementation:

```cpp
void NetworkDispatcher::_DispatchThread()
{
    while (mIsRunning)
    {
        // Swap the packet queue
        {
            std::lock_guard<std::mutex> lock(mMutex);

            if (mPacketQueue.empty())
            {
                continue;
            }

            mWorkQueue.swap(mPacketQueue);  // Safely transfer packets
        }

        // Process the packets
        for (int i = 0; i < mWorkQueue.size(); ++i)
        {
            mService->ProcessPacket(*(mWorkQueue.front()));
            mWorkQueue.pop();
        }
    }
}
```

The mutex is held only during the swap operation, reducing contention and allowing IOCP threads to enqueue packets without waiting.
