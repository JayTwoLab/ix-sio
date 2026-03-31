cmake_minimum_required(VERSION 3.10)

project(socketio_client_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Recommended: build type when using single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Try to find ixwebsocket; if not available, fetch it
find_package(ixwebsocket QUIET)
if(NOT ixwebsocket_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.2.0
  )
  FetchContent_MakeAvailable(ixwebsocket)
endif()

add_executable(socketio_client main.cpp)

target_link_libraries(socketio_client PRIVATE
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  CURL::libcurl
  Threads::Threads
  ixwebsocket::ixwebsocket
)

if(WIN32)
  # required for Winsock and BCryptGenRandom used by mbedtls (mbedcrypto)
  target_link_libraries(socketio_client PRIVATE ws2_32 bcrypt)
endif()

# Optional: enable warnings
if(MSVC)
  target_compile_options(socketio_client PRIVATE /W4 /permissive-)
else()
  target_compile_options(socketio_client PRIVATE -Wall -Wextra -Wpedantic)
endif()
