#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>

    // Windows requires WSAStartup before using sockets, and WSACleanup on shutdown. This helper struct ensures that happens.
    struct WinSockInit {
        WinSockInit() {
            WSADATA wsaData;
            int r = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (r != 0) {
                throw std::runtime_error("WSAStartup failed: " + std::to_string(r));
            }
        }
        ~WinSockInit() { WSACleanup(); }
    };
#endif

// logging with spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <nlohmann/json.hpp> // json parsing for event data and payloads

#include <curl/curl.h> // used by V2 polling handshake

#include <ixwebsocket/IXWebSocket.h> // socket.io client uses ixwebsocket for websocket communication

// Include refactored headers
#include "SioClientBase.hpp"
#include "SioClientV2.hpp"
#include "SioClientV4.hpp"

// Socket.IO v3/v4 example with direct websocket connection (no polling handshake)
static void runV4Example()
{
    spdlog::info("[Example] Starting Socket.IO v3/v4 example");

    SioClientV4 client("sio4   "); // Socket.IO v3/v4 client

    // client.getLogger()->set_level(spdlog::level::trace);
    client.getLogger()->set_level(spdlog::level::info);

    std::atomic<bool> disconnected(false); // Atomic flag tracking whether disconnected from the server

    // Register emit handler that logs on send
    client.onEmit([&client](const std::string& event, const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] Emitting {} -> {}", event, data.dump());
        }
    });

    // Register handler for disconnect
    client.onDisconnect([&client, &disconnected]() {
        if (auto lg = client.getLogger()) {
            lg->info("[App] Disconnected, scheduling cleanup");
        } 
    }); 

    // Register handler to log when "server-info" event is received
    client.on("server-info", [&client](const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] message received: {}", data.dump());
        }
    });

    // Register handler to log any received event
    client.onAny([&client](const std::string& event, const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] any event: {} -> {}", event, data.dump());
        }
    });

    try {
        client.connect("127.0.0.1", 3004, "/socket.io/");
    } catch (const std::exception &ex) {
        spdlog::error("[Example][Error] V4 connect failed: {}", ex.what());
        return;
    }

    // Wait for initial connection
    if (!SioClientBase::waitForConnect(client, 10000)) { // wait 10 seconds
        spdlog::error("[Example][Error] V4 client did not connect within timeout");
        return;
    }

    // Work loop
    for (int i = 0; i < 3; ++i) {

        // Check final connection state before starting work
        if (disconnected.load(std::memory_order_relaxed) || !client.isConnected()) {
            spdlog::info("[Example] Aborting send loop due to disconnect");
            return;
        }

        // If connected, send
        client.emit("message", nlohmann::json({{"user", "cpp_client_v4"}, {"text", "Hello from v4 client!"}}));

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    client.stop(); // close socket
    spdlog::info("[Example] Socket.IO v3/v4 example finished");
}

// Socket.IO v2 example with polling handshake to obtain sid, then websocket upgrade
static void runV2Example()
{
    spdlog::info("[Example] Starting Socket.IO v2 example");

    SioClientV2 client("sio2   "); // Socket.IO v2 client 

    // client.getLogger()->set_level(spdlog::level::trace);
    client.getLogger()->set_level(spdlog::level::info);

    std::atomic<bool> disconnected{ false }; // Atomic flag tracking whether disconnected from the server

    // Register emit handler that logs on send
    client.onEmit([&client](const std::string& event, const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] Emitting {} -> {}", event, data.dump());
        }
    });

    // Register handler for disconnect
    client.onDisconnect([&client, &disconnected](){
        if (auto lg = client.getLogger()) {
            lg->info("[App] Disconnected, cleaning up");
        }
    });

    // Register handler to log when "chat message" event is received
    client.on("chat message", [&client](const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] chat message received: {}", data.dump());
        }
    });

    // Register handler to log any received event
    client.onAny([&client](const std::string& event, const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] any event: {} -> {}", event, data.dump());
        }
    });

    try {
        client.connect("127.0.0.1", 3002, "/socket.io/");
    } catch (const std::exception &ex) {
        spdlog::error("[Example][Error] V2 connect failed: {}", ex.what());
        return;
    }

    // Wait for initial connection (10s)
    if (!SioClientBase::waitForConnect(client, 10000)) {
        spdlog::error("[Example][Error] V2 client did not connect within timeout");
        return; 
    }  

    // Work loop
    for (int i = 0; i < 3; ++i) {

        // Check final connection state before starting work
        if (disconnected.load(std::memory_order_relaxed) || !client.isConnected()) {
            spdlog::info("[Example] Aborting send loop due to disconnect");
            return; 
        }

        // If connected, send message to server
        client.emit("chat message", nlohmann::json({{"user", "cpp_client_v2"}, {"text", "Hello from v2 client!"}}));

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    client.stop(); // close socket
    spdlog::info("[Example] Socket.IO v2 example finished");
}

int main() {

    // initialize logging
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_level(spdlog::level::trace);
    // console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    // console->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(console);

#ifdef _WIN32
    WinSockInit wsi;
#endif

    // libcurl init for SioClientV2
    if (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {
        spdlog::info("[Main] libcurl global initialization succeeded");
    }
    else {
        spdlog::error("[Main][Error] libcurl global initialization failed");
        return 1;
    }

    int ret = 0;

    try {
        // Run examples separately

		runV4Example(); // socket.io v3/v4 example with direct websocket connection

        std::this_thread::sleep_for(std::chrono::seconds(1));
        spdlog::info("==============================================");

		runV2Example(); // socket.io v2 example with polling handshake

        // brief observe period
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ret = 0;
    } catch (const std::exception& ex) {
        spdlog::critical("[Fatal] {}", ex.what());
        ret = 1; // failed
    }

    curl_global_cleanup();
    return ret; 
}