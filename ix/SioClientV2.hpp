#pragma once

#include "SioClientBase.hpp"

#include <curl/curl.h> // used by V2 polling handshake
#include <utility>

class SioClientV2 : public SioClientBase {
private:
    bool _awaitingProbe = false;

    // moved libcurl helper into the class
    static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t total = size * nmemb;
        std::string* s = static_cast<std::string*>(userp);
        s->append(static_cast<char*>(contents), total);
        return total;
    }

    // httpGet is now part of SioClientV2
    static std::string httpGet(const std::string& url)
    {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::string err = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            throw std::runtime_error(std::string("curl perform failed: ") + err);
        }

        curl_easy_cleanup(curl);
        return response;
    }

    std::pair<std::string, bool> extractSidAndCheckConnect(const std::string& resp)
    {
        // find first JSON opening
        size_t start = resp.find_first_of("{[");
        if (start == std::string::npos) throw std::runtime_error("No JSON object/array found in polling response");

        char openChar = resp[start];
        char closeChar = (openChar == '{') ? '}' : ']';

        bool inString = false;
        bool escaped = false;
        int depth = 0;
        size_t end = std::string::npos;

        for (size_t i = start; i < resp.size(); ++i) {
            char c = resp[i];

            if (escaped) {
                escaped = false;
                continue;
            }

            if (c == '\\') {
                if (inString) escaped = true;
                continue;
            }

            if (c == '"') {
                inString = !inString;
                continue;
            }

            if (inString) continue;

            if (c == openChar) {
                ++depth;
            }
            else if (c == closeChar) {
                --depth;
                if (depth == 0) {
                    end = i;
                    break;
                }
            }
        }

        if (end == std::string::npos) {
            throw std::runtime_error("Could not find end of JSON in polling response");
        }

        std::string jsonPart = resp.substr(start, end - start + 1);
        auto j = nlohmann::json::parse(jsonPart);
        if (!j.contains("sid")) throw std::runtime_error("sid not found in polling response JSON");
        std::string sid = j["sid"].get<std::string>();

        // Check the trailing part (after end) for a "40" token indicating socket.io connect ack
        bool connectedFromPolling = false;
        if (end + 1 < resp.size()) {
            std::string tail = resp.substr(end + 1);
            // rough check: look for "40" as a standalone packet marker or prefixed by length framing like "2:40"
            if (tail.find("40") != std::string::npos) connectedFromPolling = true;
        }

        return { sid, connectedFromPolling };
    }

    // Perform polling handshake and set _isConnected if server already returned 40 in the polling response
    std::string pollingHandshakeGetSid(const std::string& host, int port, const std::string& path)
    {
        std::string pollingPath = ensureLeadingSlash(path);
        std::string url = "http://" + host + ":" + std::to_string(port);
        if (pollingPath == "/") url += "/";
        else url += pollingPath;
        url += (url.find('?') == std::string::npos) ? "?EIO=3&transport=polling" : "&EIO=3&transport=polling";

        if (_logger) _logger->trace("[Debug] Polling response: (requesting) {}", url);

        std::string resp = httpGet(url);
        if (_logger) _logger->trace("[Debug] Polling response: {}", resp);

        auto result = extractSidAndCheckConnect(resp);
        std::string sid = result.first;
        bool connectedFromPolling = result.second;
        if (connectedFromPolling) {
            if (_logger) _logger->trace("[Debug] Server already sent 40 over polling — marking Socket.IO connected (v2)");
            _isConnected = true;
        }
        return sid;
    }

    void setupCallbacks() override {
        _webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
            switch (msg->type) {
                case ix::WebSocketMessageType::Message:
                    if (_logger) _logger->trace("[Debug][IN] {}", msg->str);
                    this->processPacket(msg->str);
                    break;
                case ix::WebSocketMessageType::Open:
                    _websocketOpen = true;
                    if (_logger) _logger->trace("[System] WebSocket connection established (v2)");
                    // Start probe to upgrade transport to websocket if not already Socket.IO connected
                    if (!_isConnected) {
                        if (_logger) _logger->trace("[Debug] Sending probe (2probe) to server (v2)");
                        sendLogged("2probe");
                        _awaitingProbe = true;
                    } else {
                        if (_logger) _logger->trace("[Debug] Already connected via polling; will still try probe to upgrade");
                        sendLogged("2probe");
                        _awaitingProbe = true;
                    }
                    break;
                case ix::WebSocketMessageType::Close:
                    if (_logger) _logger->trace("[System] Connection closed (v2)");
                    _websocketOpen = false;
                    _isConnected = false;
                    _awaitingProbe = false;
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
        if (_logger) _logger->trace("[Debug][processPacket] engine.io type: '{}'", std::string(1, eioType));
        switch (eioType) {
            case '0':
                if (_logger) _logger->trace("[Handshake][processPacket] Received server info (v2)");
                // send Socket.IO connect if not already connected by polling
                sendLogged("40");
                break;
            case '2':
                if (_logger) _logger->trace("[Debug][processPacket] v2 Ping received, replying pong");
                sendLogged("3");
                break;
            case '3': {
                // Could be probe reply: "3probe"
                std::string payload = data.size() > 1 ? data.substr(1) : std::string();
                if (_awaitingProbe && payload.rfind("probe", 0) == 0) {
                    if (_logger) _logger->trace("[Debug][processPacket] Received probe response (3probe), sending upgrade '5'");
                    // indicate client accepts upgrade
                    sendLogged("5");
                    _awaitingProbe = false;
                    // After upgrade, mark Socket.IO connected (if not already)
                    if (!_isConnected) {
                        _isConnected = true;
                        if (_logger) _logger->trace("[Status][processPacket] Socket.io v2 connected via websocket upgrade");
                    }
                } else {
                    // regular pong
                    if (_logger) _logger->trace("[Debug][processPacket] v2 Pong or 3-message: {}", payload);
                }
                break;
            }
            case '4':
                if (data.size() > 1) {
                    char sioType = data[1];
                    if (_logger) _logger->trace("[Debug][processPacket] socket.io type: '{}'", std::string(1, sioType));
                    if (sioType == '0') {
                        _isConnected = true;
                        if (_logger) _logger->trace("[Status][processPacket] Socket.io v2 connected");
                    } else if (sioType == '1') {
                        if (_logger) _logger->trace("[Status][processPacket] Socket.io v2 disconnect packet received");
                        notifyDisconnectByServer();
                    } else if (sioType == '2') {
                        std::string payload = data.substr(2);
                        if (_logger) _logger->trace("[Event][processPacket] Received data (v2): {}", payload);
                        try {
                            nlohmann::json j = nlohmann::json::parse(payload);
                            if (j.is_array() && j.size() >= 1 && j[0].is_string()) {
                                std::string eventName = j[0].get<std::string>();
                                nlohmann::json eventData = (j.size() > 1) ? j[1] : nlohmann::json();
                                dispatchEvent(eventName, eventData);
                            }
                        } catch (const std::exception& ex) {
                            if (_logger) _logger->trace("[Error][processPacket] Failed to parse event payload: {}", ex.what());
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

public:

    SioClientV2(std::string loggerName = "") : SioClientBase(loggerName) {

        // register a UrlProvider that performs the polling handshake to obtain sid
        setUrlProvider([this](const std::string& host, int port, const std::string& path) -> std::string {
            std::string sid = pollingHandshakeGetSid(host, port, path);
            if (_logger) _logger->trace("[Debug] V2 polling handshake sid={}", sid);
             
            std::string url = "ws://" + host + ":" + std::to_string(port);
            if (path == "/") url += "/";
            else url += path;
            url += "?EIO=3&transport=websocket&sid=" + sid;
            return url;
        });
    }
    virtual ~SioClientV2() {}
};