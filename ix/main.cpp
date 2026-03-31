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
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

#include <curl/curl.h> // used by V2 polling handshake

// Include refactored headers
#include "SioClientBase.hpp"
#include "SioClientV2.hpp"
#include "SioClientV4.hpp"

#ifdef _WIN32
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

// Socket.IO v3/v4 example with direct websocket connection (no polling handshake)
static void runV4Example()
{
    spdlog::info("[Example] Starting Socket.IO v3/v4 example");

    SioClientV4 client("sio4   "); // socket.io v3/v4 클라이언트

    // client.getLogger()->set_level(spdlog::level::trace);
    client.getLogger()->set_level(spdlog::level::info);

    std::atomic<bool> disconnected(false); // 서버로부터 연결이 끊어졌는지 여부를 추적하는 원자 플래그

    // 송신 시 로그를 찍는 emit 핸들러 등록 
    client.onEmit([&client](const std::string& event, const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] Emitting {} -> {}", event, data.dump());
        }
    });

    // 연결이 끊어졌을 때 핸들러 등록
    client.onDisconnect([&client, &disconnected]() {
        if (auto lg = client.getLogger()) {
            lg->info("[App] Disconnected, scheduling cleanup");
        }
    });

    // "server-info" 이벤트 수신 시 로그를 찍는 핸들러 등록
    client.on("server-info", [&client](const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] message received: {}", data.dump());
        }
    });

    // 모든 이벤트 수신 시 로그를 찍는 핸들러 등록
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

    // 초기 연결 대기
    if (!SioClientBase::waitForConnect(client, 10000)) { // 10초 대기
        spdlog::error("[Example][Error] V4 client did not connect within timeout");
        return;
    }

    // 작업 루프
    for (int i = 0; i < 3; ++i) {

        // 작업 시작 전 최종 연결 상태 확인
        if (disconnected.load(std::memory_order_relaxed) || !client.isConnected()) {
            spdlog::info("[Example] Aborting send loop due to disconnect");
            return;
        }

        // 정상 연결 상태면 전송
        client.emit("message", nlohmann::json({{"user", "cpp_client_v4"}, {"text", "Hello from v4 client!"}}));

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    client.stop(); // 소켓 닫기
    spdlog::info("[Example] Socket.IO v3/v4 example finished");
}

// Socket.IO v2 example with polling handshake to obtain sid, then websocket upgrade
static void runV2Example()
{
    spdlog::info("[Example] Starting Socket.IO v2 example");

    SioClientV2 client("sio2   "); // socket.io v2 클라이언트 

    // client.getLogger()->set_level(spdlog::level::trace);
    client.getLogger()->set_level(spdlog::level::info);

    std::atomic<bool> disconnected{ false }; // 서버로부터 연결이 끊어졌는지 여부를 추적하는 원자 플래그

    // 송신 시 로그를 찍는 emit 핸들러 등록
    client.onEmit([&client](const std::string& event, const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] Emitting {} -> {}", event, data.dump());
        }
    });

    // 연결이 끊어졌을 때 핸들러 등록
    client.onDisconnect([&client, &disconnected](){
        if (auto lg = client.getLogger()) {
            lg->info("[App] Disconnected, cleaning up");
        }
    });

    // "chat message" 이벤트 수신 시 로그를 찍는 핸들러 등록
    client.on("chat message", [&client](const nlohmann::json& data){
        if (auto lg = client.getLogger()) {
            lg->info("[App] chat message received: {}", data.dump());
        }
    });

    // 모든 이벤트 수신 시 로그를 찍는 핸들러 등록
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

    // 초기 연결 대기 (10초)
    if (!SioClientBase::waitForConnect(client, 10000)) {
        spdlog::error("[Example][Error] V2 client did not connect within timeout");
        return; 
    }  

    // 작업 루프
    for (int i = 0; i < 3; ++i) {

        // 작업 시작 전 최종 연결 상태 확인
        if (disconnected.load(std::memory_order_relaxed) || !client.isConnected()) {
            spdlog::info("[Example] Aborting send loop due to disconnect");
            return; 
        }

        // 정상 연결 상태면 서버로 메시지 전송
        client.emit("chat message", nlohmann::json({{"user", "cpp_client_v2"}, {"text", "Hello from v2 client!"}}));

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    client.stop(); // 소켓 닫기
    spdlog::info("[Example] Socket.IO v2 example finished");
}

int main() {

    // initialize logging
    auto console = spdlog::stdout_color_mt("console");

    spdlog::set_level(spdlog::level::trace);

    // console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    // console->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(console);

    try {
        // libcurl init for SioClientV2
        curl_global_init(CURL_GLOBAL_DEFAULT);

    #ifdef _WIN32
        WinSockInit wsi;
    #endif

        // Run examples separately
        runV4Example();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        spdlog::info("==============================================");
        runV2Example();

        // brief observe period
        std::this_thread::sleep_for(std::chrono::seconds(2));

        curl_global_cleanup();
        return 0;
    } catch (const std::exception& ex) {
        spdlog::critical("[Fatal] {}", ex.what());
        curl_global_cleanup();
        return 1;
    }
}