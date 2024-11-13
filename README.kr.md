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
