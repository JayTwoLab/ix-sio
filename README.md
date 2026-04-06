# ix-sio

- `ix-sio` 프로젝트는 `C++` 기반의 `socket.io` `client`와 
- `Node.js` 기반의 `socket.io` `server` 예제들을 포함한 저장소입니다.

## 폴더 구조

- `ix/` : C++로 작성된 Socket.IO 클라이언트 구현 및 관련 소스 코드
    - `main.cpp` : 클라이언트 실행 예제
    - `SioClientBase.hpp`, `SioClientV2.hpp`, `SioClientV4.hpp` : Socket.IO 클라이언트 구현체
    - `CurlGlobal.hpp`, `WinSockInit.hpp` : 네트워크 초기화 관련 유틸리티
    - `CMakeLists.txt`, `CMakePresets.json` : CMake 빌드 설정 파일
- `sio2/` : Node.js 기반 Socket.IO v2 서버 예제
    - `server2.js` : Socket.IO v2 서버 코드
    - `package.json` : 의존성 및 실행 스크립트
- `sio3/` : Node.js 기반 Socket.IO v3 서버 예제
    - `server3.js` : Socket.IO v3 서버 코드
    - `package.json` : 의존성 및 실행 스크립트

## 주요 특징

- 다양한 버전의 Socket.IO 서버와의 호환성 테스트 가능
- C++ 클라이언트와 Node.js 서버 예제 코드 제공
- CMake를 이용한 손쉬운 빌드 환경

## 빌드 및 실행 방법

### C++ 클라이언트 (ix)

1. CMake를 이용해 빌드
   ```sh
   cd ix
   cmake -B build
   cmake --build build
   ```
2. 실행 파일을 통해 클라이언트 실행

### Node.js 서버 (sio2, sio3)

1. 각 폴더(`sio2`, `sio3`)에서 의존성 설치
   ```sh
   npm install
   ```
2. 서버 실행
   ```sh
   node server2.js   # 또는 node server3.js
   ```

