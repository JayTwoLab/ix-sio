#pragma once

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
#include <atomic>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

// Base client with common logic
class SioClientBase {
public:
    using UrlProvider = std::function<std::string(const std::string& host, int port, const std::string& path)>;
    using EmitHandler = std::function<void(const std::string& eventName, const nlohmann::json& data)>;
    using DisconnectHandler = std::function<void()>; // called when server disconnects

protected:
    ix::WebSocket _webSocket;
    bool _isConnected = false;    // Socket.IO connected (after '40' ack)
    bool _websocketOpen = false;  // raw WebSocket open

    // per-instance logger
    std::shared_ptr<spdlog::logger> _logger;

    void sendLogged(const std::string& s) {
        if (_logger) _logger->trace("[Debug][OUT] {}", s);
        _webSocket.send(s);
    }

    static std::string ensureLeadingSlash(const std::string& path) {
        if (path.empty()) return "/";
        if (path.front() == '/') return path;
        return "/" + path;
    }

private:
    std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> _eventHandlers;
    std::function<void(const std::string&, const nlohmann::json&)> _anyHandler;

    UrlProvider _urlProvider;
    EmitHandler _emitHandler;
    DisconnectHandler _disconnectHandler;

protected:
    // internal: dispatch event to registered handlers
    void dispatchEvent(const std::string& eventName, const nlohmann::json& data) {
        auto it = _eventHandlers.find(eventName);
        if (it != _eventHandlers.end()) {
            try { it->second(data); } catch (...) { /* swallow handler exceptions */ }
        }
        if (_anyHandler) {
            try { _anyHandler(eventName, data); } catch (...) { }
        }
    }

    // notify application that server disconnected (either socket.io '41' or websocket close)
    void notifyDisconnectByServer() {
        // update internal state
        _isConnected = false;
        _websocketOpen = false;
        if (_logger) _logger->trace("[System] notifyDisconnectByServer()");

        if (_disconnectHandler) {
            try { _disconnectHandler(); } catch (...) { /* swallow */ }
        }
    }

    // Allow derived classes to set up their message/open/close/error callbacks
    virtual void setupCallbacks() {}

public:
    // SioClientBase constructor: generate unique logger name if none provided
    SioClientBase(std::string loggerName = "") {
        if (loggerName.empty()) {
            using namespace std::chrono;
            auto now = system_clock::now();
            std::time_t tt = system_clock::to_time_t(now);
            std::tm tm {};
        #ifdef _WIN32
            localtime_s(&tm, &tt);
        #else
            localtime_r(&tt, &tm);
        #endif

            // auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;

            static std::atomic<uint64_t> s_counter{0};
            uint64_t id = s_counter.fetch_add(1, std::memory_order_relaxed);

            std::ostringstream oss;
            oss << "SioClientBase" 
                << "_" << id
                << "_" << std::put_time(&tm, "%Y-%m-%d_%H:%M:%S")
                ;

            loggerName = oss.str();
        }

        // Try reuse existing logger, otherwise create new, fallback to default on failure
        if (auto existing = spdlog::get(loggerName)) {
            _logger = existing;
        } else {
            try {
                _logger = spdlog::stdout_color_mt(loggerName);
            } catch (const spdlog::spdlog_ex& ex) {
                _logger = spdlog::default_logger();
                _logger->warn("Failed to create logger '{}', falling back to default logger: {}", loggerName, ex.what());
            }
        }
    }
     
    virtual ~SioClientBase() {}

    // Allow caller to inject a specific logger instance
    void setLogger(std::shared_ptr<spdlog::logger> logger) {
        _logger = std::move(logger);
    }

    std::shared_ptr<spdlog::logger> getLogger() const {
        return _logger;
    }

    // Default connect implementation uses a UrlProvider if set, otherwise defaults to EIO=4 websocket URL
    virtual void connect(const std::string& host, int port, const std::string& rawPath = "/socket.io/") {
        std::string path = ensureLeadingSlash(rawPath);
        std::string url;
        if (_urlProvider) {
            url = _urlProvider(host, port, path);
        } else {
            url = "ws://" + host + ":" + std::to_string(port);
            if (path == "/") url += "/"; else url += path;
            url += (url.find('?') == std::string::npos) ? "?EIO=4&transport=websocket" : "&EIO=4&transport=websocket";
        }

        if (_logger) _logger->trace("[Debug] Connecting to URL: {}", url);
        _webSocket.setUrl(url);
        setupCallbacks();
        _webSocket.start();
    }

    void setUrlProvider(UrlProvider p) { _urlProvider = std::move(p); }

    virtual void stop() { 
        try { 
            _webSocket.stop(); 
        }catch (std::exception& ex) {
            if (_logger) _logger->warn("[Warn] Exception while stopping WebSocket: {}", ex.what());
        }
    }

    // Emit: call registered emit-callback for processing (e.g. logging) then send packet.
    virtual void emit(const std::string& eventName, const nlohmann::json& data) {
        if (!_isConnected) {
            if (_logger) _logger->trace("[Warn] Not Socket.IO connected yet, skipping emit");
            return;
        }

        // Application-level processing: prefer user-provided handler, otherwise default to info log.
        if (_emitHandler) {
            try { _emitHandler(eventName, data); } catch (...) { /* swallow */ }
        } else {
            if (_logger) _logger->info("[App] {} -> {}", eventName, data.dump());
        }

        // Always send the socket.io packet
        nlohmann::json packet = nlohmann::json::array({eventName, data});
        std::string rawPacket = "42" + packet.dump();
        sendLogged(rawPacket);
    }

    // Public: register handlers
    void on(const std::string& eventName, std::function<void(const nlohmann::json&)> handler) {
        _eventHandlers[eventName] = std::move(handler);
    }
    void onAny(std::function<void(const std::string&, const nlohmann::json&)> handler) {
        _anyHandler = std::move(handler);
    }

    // Register an emit callback which is called whenever emit(...) is invoked.
    // The callback should be non-blocking; exceptions are swallowed.
    void onEmit(EmitHandler handler) {
        _emitHandler = std::move(handler);
    }

    // Register a disconnect callback invoked when server disconnects.
    // Note: this callback is invoked on the ixwebsocket callback thread. Avoid long blocking ops.
    void onDisconnect(DisconnectHandler handler) {
        _disconnectHandler = std::move(handler);
    }

    bool isConnected() const { return _isConnected; }
    bool websocketOpen() const { return _websocketOpen; }

    // Wait until client.isConnected() or timeout (ms)
    static bool waitForConnect(SioClientBase &client, int timeoutMs = 10000, int pollMs = 100)
    {
        int waited = 0;
        while (!client.isConnected() && waited < timeoutMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
            waited += pollMs;
        }
        return client.isConnected();
    }
};