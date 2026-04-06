#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

// Windows requires WSAStartup before using sockets, and WSACleanup on shutdown. 
// This helper struct ensures that happens.
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

#endif