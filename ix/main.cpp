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

// logging with spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// json parsing for event data and payloads
#include <nlohmann/json.hpp> 

#include <curl/curl.h> // libcurl 
#include "CurlGlobal.hpp" // RAII wrapper for curl_global_init and curl_global_cleanup

// Include refactored headers
#include "SioClientV2.hpp" // socket.io v2 client with polling handshake
#include "SioClientV4.hpp" // socket.io v3/v4 client with direct websocket connection

// Socket.IO v3/v4 example with direct websocket connection (no polling handshake)
static void runV4Example(std::shared_ptr<spdlog::logger> console)
{
    console->info("[Example] Starting Socket.IO v3/v4 example");

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
        console->error("[Example][Error] V4 connect failed: {}", ex.what());
        return;
    }

    // Wait for initial connection
    if (!SioClientBase::waitForConnect(client, 10000)) { // wait 10 seconds
        console->error("[Example][Error] V4 client did not connect within timeout");
        return;
    }

    // Work loop
    for (int i = 0; i < 3; ++i) {

        // Check final connection state before starting work
        if (disconnected.load(std::memory_order_relaxed) || !client.isConnected()) {
            console->info("[Example] Aborting send loop due to disconnect");
            return;
        }

        // If connected, send
        client.emit("message", nlohmann::json({{"user", "cpp_client_v4"}, {"text", "Hello from v4 client!"}}));

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    client.stop(); // close socket
    console->info("[Example] Socket.IO v3/v4 example finished");
}

// Socket.IO v2 example with polling handshake to obtain sid, then websocket upgrade
static void runV2Example(std::shared_ptr<spdlog::logger> console)
{
    console->info("[Example] Starting Socket.IO v2 example");

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
        console->error("[Example][Error] V2 connect failed: {}", ex.what());
        return;
    }

    // Wait for initial connection (10s)
    if (!SioClientBase::waitForConnect(client, 10000)) {
        console->error("[Example][Error] V2 client did not connect within timeout");
        return; 
    }  

    // Work loop
    for (int i = 0; i < 3; ++i) {

        // Check final connection state before starting work
        if (disconnected.load(std::memory_order_relaxed) || !client.isConnected()) {
            console->info("[Example] Aborting send loop due to disconnect");
            return; 
        }

        // If connected, send message to server
        client.emit("chat message", nlohmann::json({{"user", "cpp_client_v2"}, {"text", "Hello from v2 client!"}}));

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    client.stop(); // close socket
    console->info("[Example] Socket.IO v2 example finished");
}

int main() {

    // initialize logging
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_level(spdlog::level::trace);
    // console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    // console->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(console);

#ifdef _WIN32
	// Ensure WinSock is initialized before any socket operations 
    // (including those in WebSocket++ or libcurl)
    WinSockInit wsi;
#endif

    std::unique_ptr<CurlGlobal> curlInit = nullptr;
    try {
        curlInit = std::make_unique<CurlGlobal>();
		// CurlGlobal will automatically clean up when going out of scope at the end of main
    } catch (const std::exception& ex) {
        console->critical("[Fatal] CurlGlobal initialization failed: {}", ex.what());
        return 1;
    }

    try { // Run examples separately

		runV4Example(console); // socket.io v3/v4 example with direct websocket connection

        std::this_thread::sleep_for(std::chrono::seconds(1));
        console->info("==============================================");

		runV2Example(console); // socket.io v2 example with polling handshake
        
        std::this_thread::sleep_for(std::chrono::seconds(2)); // brief observe period
    } catch (const std::exception& ex) {
        console->critical("[Fatal] {}", ex.what());
        return 2; 
    }

    return 0; 
}