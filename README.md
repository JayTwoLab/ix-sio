# ix-sio

- `C++` 기반의 [`socket.io`](https://socket.io/) `client`
   - [`IXWebSocket`](https://github.com/machinezone/IXWebSocket) 기반의 `Socket.IO` 클라이언트 구현체 제공
- `Node.js` 기반의 `socket.io` `server` 예제

## 폴더 구조

- `ix/` : C++로 작성된 Socket.IO 클라이언트 구현 및 관련 소스 코드
    - `main.cpp` : 클라이언트 실행 예제
    - `SioClientBase.hpp`, `SioClientV2.hpp`, `SioClientV4.hpp` : Socket.IO 클라이언트 구현체
    - `CurlGlobal.hpp`, `WinSockInit.hpp` : 네트워크 초기화 관련 유틸리티
    - `CMakeLists.txt`, `CMakePresets.json` : CMake 빌드 설정 파일
- `sio2/` : Node.js 기반 Socket.IO v2 서버 예제
    - `server2.js` : Socket.IO v2 서버 코드
    - `package.json` : 의존성 및 실행 스크립트
- `sio3/` : Node.js 기반 Socket.IO v3/v4 서버 예제
    - `server3.js` : Socket.IO v3/v4 서버 코드
    - `package.json` : 의존성 및 실행 스크립트

## 주요 특징

- 다양한 버전의 Socket.IO 서버와의 호환성 테스트 가능
- C++ 클라이언트와 Node.js 서버 예제 코드 제공
- CMake를 이용한 손쉬운 빌드 환경

## 빌드 및 실행 방법

### `C++` 클라이언트 (`ix`)

- (1) CMake를 이용해 빌드
   ```sh
     cd ix
     cmake -B build
     cmake --build build
   ```
- (2) 실행 파일을 통해 클라이언트 실행

### 의존성 설치 방법 (CURL, spdlog, nlohmann_json, ixwebsocket)

#### Windows (MSVC + vcpkg)

1. [vcpkg](https://vcpkg.io/) 설치 및 환경설정
2. vcpkg로 패키지 설치:
    ```sh
    vcpkg install curl spdlog nlohmann-json ixwebsocket
    ```
3. CMake 빌드 시 vcpkg toolchain 파일 지정:
    ```sh
    cmake -B build -DCMAKE_TOOLCHAIN_FILE="<vcpkg-root>/scripts/buildsystems/vcpkg.cmake"
    cmake --build build
    ```
    - `<vcpkg-root>`는 vcpkg가 설치된 경로로 교체

#### Linux (gcc + apt/yum)

- **Debian/Ubuntu (apt):**
   ```sh
   sudo apt update
   sudo apt install libcurl4-openssl-dev libspdlog-dev nlohmann-json3-dev libixwebsocket-dev
   ```
- **Fedora (dnf):**
   ```sh
   sudo dnf install libcurl-devel spdlog-devel nlohmann-json-devel 
   ```
- **CentOS/RHEL (yum):**
   ```sh
   sudo yum install libcurl-devel spdlog-devel nlohmann-json-devel 
   ```


#### Rocky Linux에서 ixwebsocket 설치

Rocky Linux 등 일부 RHEL 계열 배포판에는 `ixwebsocket-devel` 패키지가 제공되지 않습니다.
이 경우, 소스에서 직접 빌드해야 합니다:

1. 의존성 설치
   ```sh
   sudo yum install libcurl-devel spdlog-devel nlohmann-json-devel
   ```
2. IXWebSocket 소스 다운로드 및 빌드
   ```sh
   git clone https://github.com/machinezone/IXWebSocket.git
   cd IXWebSocket
   mkdir build && cd build
   cmake ..
   make -j
   sudo make install
   ```
   - 또는 특정한 릴리즈 버전을 다운로드 받아서 설치하여도 된다. https://github.com/machinezone/IXWebSocket/releases 
3. CMake 빌드 시 IXWebSocket이 설치된 경로가 자동으로 탐지되지 않으면, `CMAKE_PREFIX_PATH`에 설치 경로를 추가:
   ```sh
   cmake -B build -DCMAKE_PREFIX_PATH="/usr/local"
   ```

#### 참고
- CMake에서 각 라이브러리는 `find_package`로 자동 탐지됩니다.
- vcpkg/패키지 매니저를 사용하면 별도 소스 빌드 없이 간편하게 설치할 수 있습니다.

### `Node.js` 서버 (`sio2`, `sio3`)

- (1) 각 폴더(`sio2`, `sio3`)에서 의존성 설치
   ```sh
     npm install
   ```
- (2) 서버 실행
   ```sh
     node server2.js  # 또는 node server3.js
   ```

