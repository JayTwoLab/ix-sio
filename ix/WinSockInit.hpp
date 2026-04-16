#pragma once

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <string>

// Windows requires WSAStartup before using sockets, and WSACleanup on shutdown. 
// This helper struct ensures that happens.
namespace j2::network {

// WinSockInit : RAII wrapper for WSAStartup and WSACleanup
//
// Example usage:
// #ifdef _WIN32
//     try {
//         j2::network::WinSockInit wsi;
//     } catch (const std::exception& ex) {
//         std::cerr << "[Fatal] WinSock initialization failed: " << ex.what() << std::endl;
//         return 1;
//     }
// #endif
// 
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

#endif // _WIN32