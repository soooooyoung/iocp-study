# IOCP Study

## Tutorial Branches

These branches follow steps [IOCP Tutorial Repo](https://github.com/jacking75/edu_cpp_IOCP.git) by [jacking75](https://github.com/jacking75) for learning purposes

[단계 별로 IOCP 실습](https://github.com/jacking75/edu_cpp_IOCP.git) 진행 과정을 기록합니다.

- [Step 1-2](https://github.com/soooooyoung/iocp-study/tree/IOCP_01_02)
- [Step 3](https://github.com/soooooyoung/iocp-study/tree/IOCP_03)
- [Step 4](https://github.com/soooooyoung/iocp-study/tree/IOCP_04)
- [Step 5](https://github.com/soooooyoung/iocp-study/tree/IOCP_05)
- [Step 6](https://github.com/soooooyoung/iocp-study/tree/IOCP_06)
- [Step 7](https://github.com/soooooyoung/iocp-study/tree/IOCP_07)

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

| 구성 요소      | 역할                                                                                                                    |
| :------------- | :---------------------------------------------------------------------------------------------------------------------- |
| IOCPHandler    | I/O 완료 포트(IOCP) 작업을 관리하며, I/O 이벤트를 처리하기 위한 작업자 스레드와 클라이언트 IOCP 핸들 등록을 포함합니다. |
| NetworkClient  | 클라이언트의 연결을 수신 대기하며, 포트에 바인딩하고 연결 수락을 비동기적으로 처리합니다.                               |
| \*ListenClient | 클라이언트 연결 세션을 나타내며, 세션 ID와 소켓을 유지하고 송신 작업을 처리합니다.                                      |
| NetworkManager | 전체 네트워크 작업 (클라이언트 등록 및 패킷 전송 등) 을 조율하며 IOCPHandler와 상호 작용합니다.                         |
| SimpleCore     | 메모리 풀과 싱글톤 패턴과 같은 네트워크 라이브러리 전반에서 사용되는 기본 포함 파일 및 유틸리티 클래스를 제공합니다.    |
| NetworkContext | 각 I/O 작업의 컨텍스트를 나타내며, 버퍼와 연결 세부 정보를 유지 관리합니다.                                             |

\*Inherits **NetworkClient**

#### Server Logic
