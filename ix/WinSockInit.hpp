#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <string>

// Windows requires WSAStartup before using sockets, and WSACleanup on shutdown. 
// This helper struct ensures that happens.
namespace j2 {
namespace network {

struct WinSockInit {
    WinSockInit() {
        WSADATA wsaData;
        int r = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (r != 0) {
            throw std::runtime_error("WSAStartup failed: " + std::to_string(r));
        }
    }

    ~WinSockInit() { 
        WSACleanup(); 
    }
};

} // namespace network
} // namespace j2

#endif