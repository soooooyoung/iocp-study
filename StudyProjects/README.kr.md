# IOCP 공부

[![en](https://img.shields.io/badge/lang-en-yellow.svg)](README.md)
[![kr](https://img.shields.io/badge/lang-kr-red.svg)](README.kr.md)

[단계 별로 IOCP 실습](https://github.com/jacking75/edu_cpp_IOCP.git) 진행 과정을 기록합니다.

## 분기별 작업 브랜치

| 단계                                                                 | 설명                                        |
| :------------------------------------------------------------------- | :------------------------------------------ |
| [1-2단계](https://github.com/soooooyoung/iocp-study/tree/IOCP_01_02) | IOCP를 활용한 Echo 서버 생성                |
| [3단계](https://github.com/soooooyoung/iocp-study/tree/IOCP_03)      | 네트워크 처리 로직과 애플리케이션 로직 분리 |
| [4단계](https://github.com/soooooyoung/iocp-study/tree/IOCP_04)      | 개별 스레드에서의 작업 분리                 |
| [5단계](https://github.com/soooooyoung/iocp-study/tree/IOCP_05)      | 버퍼를 통한 **\*1-Send** 구현               |
| [6단계](https://github.com/soooooyoung/iocp-study/tree/IOCP_06)      | 큐 기반 **\*1-Send** 구현                   |
| [7단계](https://github.com/soooooyoung/iocp-study/tree/IOCP_07)      | 비동기 Accept 사용                          |

**\*1-Send**: 한 번의 전송 작업이 완료되기를 기다렸다가 다음 작업을 시작하도록 구현하여 I/O 처리가 순서대로 겹치지 않게 보장하는 로직.

## 기능 개선 구현

튜토리얼에서 배운 개념과 구조를 활용하여 확장된 IOCP 서버를 구현합니다.

### 서버 구성

#### 네트워크 라이브러리

| 구성 요소      | 역할                                                                                                                 |
| :------------- | :------------------------------------------------------------------------------------------------------------------- |
| IOCPHandler    | I/O 작업을 관리하며, I/O 이벤트를 처리하기 위한 작업자 스레드와 클라이언트 IOCP 핸들 등록을 포함합니다.              |
| NetworkClient  | 연결된 클라이언트 세션을 나타내며, 세션 ID와 소켓을 관리합니다.                                                      |
| \*ListenClient | 클라이언트 연결을 대기하고 비동기 방식으로 수락을 처리합니다.                                                        |
| NetworkManager | 전체 네트워크 작업 (클라이언트 등록 및 패킷 전송 등) 을 조율하며 IOCPHandler와 상호 작용합니다.                      |
| SimpleCore     | 메모리 풀과 싱글톤 패턴과 같은 네트워크 라이브러리 전반에서 사용되는 기본 포함 파일 및 유틸리티 클래스를 제공합니다. |
| NetworkContext | 각 I/O 작업의 컨텍스트를 나타내며, 버퍼와 연결 세부 정보를 유지 관리합니다.                                          |

\*Inherits **NetworkClient**

#### 서버 로직

![](images/아완벽히이해했어.jpg)

### IOCP에서 사용자 정의 컨텍스트 사용

I/O operation 완료 시에 NetworkContext 객체와 OVERLAPPED 구조체의 호환성을 유지하면서 추가 데이터를 함께 전달 해야 했습니다.

#### 초기 구현:

NetworkContext 클래스에 `WSAOVERLAPPED` 타입의 멤버 변수를 포함시켰습니다

```cpp
class NetworkContext
{
public:
    WSAOVERLAPPED mWsaOverlapped = { 0, };
    //...
};
```

이 접근 방식은 비동기 I/O 작업을 WSAOVERLAPPED 구조체와 연관시키는 것을 가능하게 했지만, 버퍼 이외의 추가적인 컨텍스트 정보가 필요할 때는 한계가 있었습니다.

#### 수정된 구현:

이를 해결하기 위해, NetworkContext 클래스를 OVERLAPPED로부터 상속받도록 수정했습니다:

```cpp
class NetworkContext : public OVERLAPPED
{
private:
    std::array<std::uint8_t, 8096> mBuffer;
    int mReadPos = 0;
    int mWritePos = 0;
public:
    //...
};
```

이러한 상속은 메모리 정렬을 보장하여 시스템이 OVERLAPPED 필드에 직접 접근할 수 있게 합니다. 또한, 포인터 연산 없이도 나머지 컨텍스트에 접근할 수 있게 되었습니다. 그러나 이러한 상속은 NetworkContext와 OVERLAPPED 간의 의미적 관계가 없기 때문에 상속의 오용으로 간주될 수 있습니다.

### IOThread에서 패킷 직렬화 및 역직렬화 처리

수신된 패킷의 역직렬화는 IOCP 스레드에서 직접 수행됩니다.

> **\_HandleReceive**: 네트워크 수신 작업 완료를 처리합니다

```cpp
void NetworkManager::_HandleReceive(NetworkClient& client, NetworkContext& context, int transferred)
{
	// 유효한 바이트 수인지 확인
	if (transferred <= 0)
	{
		printf_s("_HandleReceive Error: %d\n", WSAGetLastError());
		return;
	}

	// 내부 버퍼 정보를 업데이트
	if (false == context.Write(transferred))
	{
		printf_s("_HandleReceive Error: Failed to Write\n");
		return;
	}

	// 패킷 역직렬화
	// 수신된 raw 데이터를 구조화된 NetworkPacket으로 변환
	std::unique_ptr<NetworkPacket> packet = client.GetPacket();

	if (nullptr == packet)
	{
		printf_s("_HandleReceive Error: Failed to GetPacket\n");
		return;
	}

	packet->Header.SessionID = client.GetSessionID();
	mDispatcher->PushPacket(std::move(packet));

	// 다음 수신 작업 준비
	client.Receive();
}
```

IOCP Thread에서 raw 데이터를 직접 처리하면 I/O 작업으로 인해 메모리에 이미 로드된 데이터를 그대로 활용할 수 있습니다 (cache locality). 이 방식은 데이터를 별도의 워커 스레드로 전달할 경우 발생하는 캐시 재로딩 작업을 방지하여 메모리 접근 지연(latency)을 줄여줍니다.

고빈도, 소규모 패킷 처리에서는 데이터를 다른 스레드로 전달하는 오버헤드를 방지함으로써 전반적인 처리량(throughput)을 향상시킬 수 있습니다. 더 큰 패킷의 경우 복잡한 처리가 필요할 수 있어 IOCP 스레드가 과부하될 위험이 있습니다.

변환이 완료된 패킷인 이후 `NetworkDispatcher`에 전달되어 애플리케이션 계층 서비스로 라우팅 됩니다.

### 패킷 큐 이벤트 관리

패킷을 NetworkDispatcher에 푸시하면, 락을 걸고 패킷을 큐로부터 직접 하나씩 처리하였습니다. 이 방식은 패킷 처리 동안 IOCP 스레드들이 패킷 큐에 추가 패킷을 추가하는 것을 막아 성능적인 우려가 있었습니다.

이 문제를 해결하기 위해, 임계 구역 밖에서 패킷을 처리할 수 있도록 임시로 패킷을 보관하는 보조 작업 큐(work queue)를 도입하는 방식으로 큐 관리 로직을 개선했습니다.

#### 초기 구현:

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

#### 개선된 구현:

```csharp
void NetworkDispatcher::_DispatchThread()
{
    while (mIsRunning)
    {
        // 패킷 큐 스와핑
        {
            std::lock_guard<std::mutex> lock(mMutex);

            if (mPacketQueue.empty())
            {
                continue;
            }

            mWorkQueue.swap(mPacketQueue);  // 패킷을 안전하게 이동
        }

        // 패킷 처리
        for (int i = 0; i < mWorkQueue.size(); ++i)
        {
            mService->ProcessPacket(*(mWorkQueue.front()));
            mWorkQueue.pop();
        }
    }
}

```

이렇게 하면 뮤텍스는 스와핑 동작 중에만 잠기므로, 경합을 줄이고 IOCP 스레드가 대기 없이 패킷을 큐에 추가할 수 있게 됩니다.
