#pragma once

#include "SioClientBase.hpp"
 
namespace j2::network {

class SioClientV4 : public SioClientBase {
private:
    void setupCallbacks() override {
        _webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
            switch (msg->type) {
                case ix::WebSocketMessageType::Message:
                    if (_logger) _logger->trace("[Debug][IN] {}", msg->str);
                    this->processPacket(msg->str);
                    break;
                case ix::WebSocketMessageType::Open:
                    _websocketOpen = true;
                    if (_logger) _logger->trace("[System] WebSocket connection established (v4)");
                    break;
                case ix::WebSocketMessageType::Close:
                    if (_logger) _logger->trace("[System] Connection closed (v4)");
                    _websocketOpen = false;
                    _isConnected = false;
                    // inform application of server-side close
                    notifyDisconnectByServer();
                    break;
                case ix::WebSocketMessageType::Error:
                    if (_logger) _logger->trace("[Error] {}", msg->errorInfo.reason);
                    break;
                default: break;
            }
        });
    }

    void processPacket(const std::string& data) {
        if (data.empty()) return;
        char eioType = data[0];
        if (_logger) _logger->trace("[Debug] engine.io type: '{}'", std::string(1, eioType));
        switch (eioType) {
            case '0':
                if (_logger) _logger->trace("[Handshake] Received server info (v4)");
                sendLogged("40");
                break;
            case '2':
                if (_logger) _logger->trace("[Debug] v4 Ping received, replying pong");
                sendLogged("3");
                break;
            case '4':
                if (data.size() > 1) {
                    char sioType = data[1];
                    if (_logger) _logger->trace("[Debug] socket.io type: '{}'", std::string(1, sioType));
                    if (sioType == '0') {
                        _isConnected = true;
                        if (_logger) _logger->trace("[Status] Socket.io v4 connected");
                    } else if (sioType == '1') {
                        if (_logger) _logger->trace("[Status] Socket.io v4 disconnect packet received");
                        notifyDisconnectByServer();
                    } else if (sioType == '2') {
                        std::string payload = data.substr(2);
                        if (_logger) _logger->trace("[Event] Received data (v4): {}", payload);
                        try {
                            nlohmann::json j = nlohmann::json::parse(payload);
                            if (j.is_array() && j.size() >= 1 && j[0].is_string()) {
                                std::string eventName = j[0].get<std::string>();
                                nlohmann::json eventData = (j.size() > 1) ? j[1] : nlohmann::json();
                                dispatchEvent(eventName, eventData);
                            }
                        } catch (const std::exception& ex) {
                            if (_logger) _logger->trace("[Error] Failed to parse event payload: {}", ex.what());
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

public:
    SioClientV4(std::string loggerName = "") : SioClientBase(loggerName) { 

        // default provider for v4 websocket URL
        setUrlProvider([](const std::string& host, int port, const std::string& path) -> std::string {
            std::string url = "ws://" + host + ":" + std::to_string(port);
            if (path == "/") url += "/";
            else url += path;
            url += (url.find('?') == std::string::npos) ? "?EIO=4&transport=websocket" : "&EIO=4&transport=websocket";
            return url;
        });
    }
    virtual ~SioClientV4() {}
};

} // namespace j2::network