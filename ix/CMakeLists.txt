cmake_minimum_required(VERSION 3.26)

project(socketio_client_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Recommended: build type when using single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

find_package(Threads REQUIRED)
find_package(CURL REQUIRED) # libcurl(curl) https://curl.se/libcurl/
find_package(spdlog REQUIRED) # spdlog https://githunb.com/gabime/spdlog
find_package(nlohmann_json REQUIRED) # nlohmann json https://github.com/nlohmann/json
find_package(ixwebsocket REQUIRED) # ixwebsocket https://github.com/machinezone/IXWebSocket

#find_package(ixwebsocket QUIET)
# Try to find ixwebsocket; if not available, fetch it
# if(NOT ixwebsocket_FOUND)
#   include(FetchContent)
#   FetchContent_Declare(
#     ixwebsocket
#     GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
#     GIT_TAG v11.2.0
#   )
#   FetchContent_MakeAvailable(ixwebsocket)
# endif()

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
  # ws2_32 is winsock, required for Windows socket programming
  # bcrypt is required for mbedtls's random number generation on Windows
endif()

# Optional: enable warnings
if(MSVC)
  # Use /W4 for maximum warnings and /permissive- to disable non-standard extensions
  # This ensures that we get the most out of the compiler's warning system while maintaining compatibility with standard C++.
  target_compile_options(socketio_client PRIVATE /W4 /permissive-)
  foreach(var
    CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
    if(DEFINED ${var})
      string(REGEX REPLACE "/W[0-4]" "/W4" _newflags "${${var}}")
      set(${var} "${_newflags}")
    endif()
  endforeach()
else()
  target_compile_options(socketio_client PRIVATE -Wall -Wextra -Wpedantic)
endif()
