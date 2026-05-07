# ix-sio

> [Korean](README.ko.md)

- `C++` based [`socket.io`](https://socket.io/) `client`
   - Provides a [`Socket.IO`](https://socket.io/) client implementation based on [`IXWebSocket`](https://github.com/machinezone/IXWebSocket)
- `Node.js` based `socket.io` `server` examples

## Folder Structure

- [`ix/`](https://github.com/JayTwoLab/ix-sio/tree/main/ix): C++ [`Socket.IO`](https://socket.io/) client implementation and related source code
    - [`main.cpp`](https://github.com/JayTwoLab/ix-sio/blob/main/ix/main.cpp): Client execution example
    - `SioClientBase.hpp`, `SioClientV2.hpp`, `SioClientV4.hpp`: Socket.IO client implementations
    - `CurlGlobal.hpp`, `WinSockInit.hpp`: Network initialization utilities
    - `CMakeLists.txt`, `CMakePresets.json`: CMake build configuration files
- [`sio2/`](https://github.com/JayTwoLab/ix-sio/tree/main/sio2): [Node.js](https://nodejs.org) based [`Socket.IO`](https://socket.io/) v2 server example
    - [`server2.js`](https://github.com/JayTwoLab/ix-sio/blob/main/sio2/server2.js): [`Socket.IO`](https://socket.io/) v2 server code
    - `package.json`: Dependencies and run scripts
- [`sio3/`](https://github.com/JayTwoLab/ix-sio/tree/main/sio3): [Node.js](https://nodejs.org) based [`Socket.IO`](https://socket.io/) v3/v4 server example
    - [`server3.js`](https://github.com/JayTwoLab/ix-sio/blob/main/sio3/server3.js): [`Socket.IO`](https://socket.io/) v3/v4 server code
    - `package.json`: Dependencies and run scripts

## Key Features

- Compatibility testing with various versions of Socket.IO servers
- Provides C++ client and Node.js server example code
- Easy build environment using CMake

## Build and Run Instructions

### `C++` Client (`ix`)

- (1) Build using CMake
   ```sh
     cd ix
     cmake -B build
     cmake --build build
   ```
- (2) Run the client using the executable

### Dependency Installation (CURL, spdlog, nlohmann_json, ixwebsocket)

#### Windows (MSVC + vcpkg)

1. Install and set up [vcpkg](https://vcpkg.io/)
2. Install packages with vcpkg:
    ```sh
    vcpkg install curl spdlog nlohmann-json ixwebsocket
    ```
3. Specify the vcpkg toolchain file when building with CMake:
    ```sh
    cmake -B build -DCMAKE_TOOLCHAIN_FILE="<vcpkg-root>/scripts/buildsystems/vcpkg.cmake"
    cmake --build build
    ```
    - Replace `<vcpkg-root>` with your vcpkg installation path

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

#### Installing ixwebsocket on Rocky Linux

Some RHEL-based distributions like Rocky Linux do not provide the `ixwebsocket-devel` package.
In this case, you need to build from source:

1. Install dependencies
   ```sh
   sudo yum install libcurl-devel spdlog-devel nlohmann-json-devel
   ```
2. Download and build IXWebSocket from source
   ```sh
   git clone https://github.com/machinezone/IXWebSocket.git
   cd IXWebSocket
   mkdir build && cd build
   cmake ..
   make -j
   sudo make install
   ```
   - Or download and install a specific release version: https://github.com/machinezone/IXWebSocket/releases 
3. If CMake does not automatically detect the IXWebSocket installation path, add the install path to `CMAKE_PREFIX_PATH`:
   ```sh
   cmake -B build -DCMAKE_PREFIX_PATH="/usr/local"
   # or
   cmake -DCMAKE_INSTALL_PREFIX=/your/custom/path ..
   ```

#### Notes
- Each library is automatically detected with `find_package` in CMake.
- Using vcpkg or a package manager allows for easy installation without building from source.

### `Node.js` Server (`sio2`, `sio3`)

- (1) Install dependencies in each folder (`sio2`, `sio3`)
   ```sh
     npm install
   ```
- (2) Run the server
   ```sh
     node server2.js  # or node server3.js
   ```
