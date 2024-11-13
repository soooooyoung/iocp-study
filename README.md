# IOCP Study

[![en](https://img.shields.io/badge/lang-en-yellow.svg)](README.md)
[![kr](https://img.shields.io/badge/lang-kr-red.svg)](README.kr.md)

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
