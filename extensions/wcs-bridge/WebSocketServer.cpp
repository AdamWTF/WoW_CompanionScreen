#include "WebSocketServer.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstring>
#include <exception>
#include <iterator>
#include <vector>

namespace wcs_bridge
{
    namespace
    {
        constexpr size_t kMaxHeader = 8192;

        std::string Lower(std::string value) { for (char& c : value) c = char(std::tolower(static_cast<unsigned char>(c))); return value; }
        std::string Trim(std::string value)
        {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
            return value;
        }

        bool SendAll(SOCKET socket, const char* data, size_t size)
        {
            while (size)
            {
                const int count = send(socket, data, int((std::min)(size, size_t(INT_MAX))), 0);
                if (count <= 0) return false; data += count; size -= size_t(count);
            }
            return true;
        }

        std::string Base64(const unsigned char* data, size_t size)
        {
            static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result; result.reserve((size + 2) / 3 * 4);
            for (size_t i = 0; i < size; i += 3)
            {
                const uint32_t value = uint32_t(data[i]) << 16 | (i + 1 < size ? uint32_t(data[i + 1]) << 8 : 0) | (i + 2 < size ? data[i + 2] : 0);
                result.push_back(table[(value >> 18) & 63]); result.push_back(table[(value >> 12) & 63]);
                result.push_back(i + 1 < size ? table[(value >> 6) & 63] : '='); result.push_back(i + 2 < size ? table[value & 63] : '=');
            }
            return result;
        }

        bool Sha1(std::string_view source, unsigned char output[20])
        {
            BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr;
            DWORD objectSize = 0, returned = 0; std::vector<unsigned char> object;
            bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA1_ALGORITHM, nullptr, 0) == 0;
            if (ok) ok = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof objectSize, &returned, 0) == 0;
            if (ok) { object.resize(objectSize); ok = BCryptCreateHash(algorithm, &hash, object.data(), objectSize, nullptr, 0, 0) == 0; }
            if (ok) ok = BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(source.data())), ULONG(source.size()), 0) == 0;
            if (ok) ok = BCryptFinishHash(hash, output, 20, 0) == 0;
            if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0); return ok;
        }

        bool Upgrade(SOCKET socket)
        {
            DWORD timeout = 2500; setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof timeout);
            std::string request; std::array<char, 2048> buffer{};
            while (request.find("\r\n\r\n") == std::string::npos && request.size() < kMaxHeader)
            {
                const int count = recv(socket, buffer.data(), int(buffer.size()), 0); if (count <= 0) return false; request.append(buffer.data(), size_t(count));
            }
            const size_t firstEnd = request.find("\r\n"); if (firstEnd == std::string::npos || request.substr(0, firstEnd) != "GET /wcs HTTP/1.1") return false;
            std::string key, upgrade, connection, version; size_t line = firstEnd + 2;
            while (line < request.size())
            {
                const size_t end = request.find("\r\n", line); if (end == std::string::npos || end == line) break;
                const size_t colon = request.find(':', line);
                if (colon != std::string::npos && colon < end)
                {
                    const std::string name = Lower(Trim(request.substr(line, colon - line))); const std::string value = Trim(request.substr(colon + 1, end - colon - 1));
                    if (name == "sec-websocket-key") key = value; else if (name == "sec-websocket-version") version = value;
                    else if (name == "upgrade") upgrade = Lower(value); else if (name == "connection") connection = Lower(value);
                }
                line = end + 2;
            }
            if (key.empty() || version != "13" || upgrade != "websocket" || connection.find("upgrade") == std::string::npos) return false;
            unsigned char digest[20]{}; if (!Sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", digest)) return false;
            const std::string response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + Base64(digest, 20) + "\r\n\r\n";
            timeout = 0; setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof timeout);
            return SendAll(socket, response.data(), response.size());
        }

        bool SendFrame(SOCKET socket, uint8_t opcode, std::string_view payload)
        {
            std::array<unsigned char, 10> header{}; size_t count = 2; header[0] = 0x80 | opcode;
            if (payload.size() < 126) header[1] = static_cast<unsigned char>(payload.size());
            else if (payload.size() <= 65535) { header[1] = 126; header[2] = uint8_t(payload.size() >> 8); header[3] = uint8_t(payload.size()); count = 4; }
            else { header[1] = 127; for (int i = 0; i < 8; ++i) header[2 + i] = uint8_t(uint64_t(payload.size()) >> (56 - i * 8)); count = 10; }
            return SendAll(socket, reinterpret_cast<char*>(header.data()), count) && SendAll(socket, payload.data(), payload.size());
        }

        struct FrameReader
        {
            std::vector<unsigned char> bytes;
            std::string fragmented;
            uint8_t fragmentOpcode = 0;

            // 0 incomplete, 1 text, 2 ping, 3 pong, 4 close, 5 fragment consumed, -1 invalid
            int Next(std::string& payload)
            {
                if (bytes.size() < 2) return 0;
                const bool fin = (bytes[0] & 0x80) != 0, masked = (bytes[1] & 0x80) != 0;
                const uint8_t opcode = bytes[0] & 15; uint64_t length = bytes[1] & 127; size_t at = 2;
                if ((bytes[0] & 0x70) || !masked) return -1;
                if (length == 126) { if (bytes.size() < 4) return 0; length = uint64_t(bytes[2]) << 8 | bytes[3]; at = 4; }
                else if (length == 127) { if (bytes.size() < 10) return 0; length = 0; for (int i = 0; i < 8; ++i) length = length << 8 | bytes[2 + i]; at = 10; }
                if (length > kMaxMessageBytes || bytes.size() < at + 4 + length) return length > kMaxMessageBytes ? -1 : 0;
                const unsigned char* mask = bytes.data() + at; at += 4; payload.resize(size_t(length));
                for (size_t i = 0; i < payload.size(); ++i) payload[i] = char(bytes[at + i] ^ mask[i & 3]);
                bytes.erase(bytes.begin(), bytes.begin() + ptrdiff_t(at + length));
                if (opcode >= 8) { if (!fin || length > 125) return -1; return opcode == 8 ? 4 : opcode == 9 ? 2 : opcode == 10 ? 3 : -1; }
                if (opcode == 1)
                {
                    if (fragmentOpcode) return -1; if (fin) return ValidUtf8(payload) ? 1 : -1;
                    fragmentOpcode = opcode; fragmented = std::move(payload); return 5;
                }
                if (opcode == 0 && fragmentOpcode)
                {
                    if (fragmented.size() + payload.size() > kMaxMessageBytes) return -1; fragmented += payload;
                    if (!fin) return 5; payload = std::move(fragmented); fragmentOpcode = 0; return ValidUtf8(payload) ? 1 : -1;
                }
                return -1;
            }
        };

        std::string StringField(const json::Value& root, const char* name)
        {
            const auto* value = root.Find(name); const auto* string = value ? value->String() : nullptr; return string ? *string : std::string{};
        }
    }

    WebSocketServer::WebSocketServer(PairingManager& pairing, CommandSink commands, SnapshotSource snapshot, NoticeSink notices)
        : pairing_(pairing), commands_(std::move(commands)), snapshot_(std::move(snapshot)), notices_(std::move(notices)) {}
    WebSocketServer::~WebSocketServer() { Stop(); }

    bool WebSocketServer::Start(std::string address, uint16_t port)
    {
        if (running_.exchange(true)) return false; address_ = std::move(address); port_ = port;
        try { thread_ = std::thread(&WebSocketServer::Run, this); return true; }
        catch (...) { running_ = false; return false; }
    }

    void WebSocketServer::Stop()
    {
        running_ = false; disconnect_ = true; if (thread_.joinable()) thread_.join();
    }

    bool WebSocketServer::Send(const json::Value& message, bool replaceable)
    {
        if (!connected_.load()) return true;
        const auto* typeValue = message.Find("type");
        const auto* type = typeValue ? typeValue->String() : nullptr;
        const bool snapshot = replaceable && type && *type == "state.snapshot";
        const std::string encoded = json::Dump(message); std::lock_guard lock(outboundMutex_);
        if (snapshot)
            outbound_.erase(std::remove_if(outbound_.begin(), outbound_.end(), [](const Outbound& item) { return item.replaceable; }), outbound_.end());
        else if (replaceable && outbound_.size() > 192)
        {
            const auto it = std::find_if(outbound_.rbegin(), outbound_.rend(), [](const Outbound& item) { return item.replaceable; });
            if (it != outbound_.rend()) outbound_.erase(std::next(it).base());
        }
        if (outbound_.size() >= 256) return false; outbound_.push_back(Outbound{encoded, replaceable}); return true;
    }

    void WebSocketServer::DisconnectClient() { disconnect_ = true; }
    std::string WebSocketServer::Address() const { return address_; }

    void WebSocketServer::Run()
    {
        try { RunImpl(); }
        catch (const std::exception& error) { if (notices_) notices_(std::string("network service stopped after internal error: ") + error.what()); }
        catch (...) { if (notices_) notices_("network service stopped after an unknown internal error"); }
        if (connected_.exchange(false)) commands_(Command{CommandKind::ReleaseAll});
        listening_ = false; running_ = false;
    }

    void WebSocketServer::RunImpl()
    {
        WSADATA data{}; if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { if (notices_) notices_("Winsock startup failed"); running_ = false; return; }
        SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener == INVALID_SOCKET) { if (notices_) notices_("listener socket creation failed"); WSACleanup(); running_ = false; return; }
        BOOL exclusive = TRUE; setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof exclusive);
        sockaddr_in endpoint{}; endpoint.sin_family = AF_INET; endpoint.sin_port = htons(port_);
        if (inet_pton(AF_INET, address_.c_str(), &endpoint.sin_addr) != 1 || bind(listener, reinterpret_cast<sockaddr*>(&endpoint), sizeof endpoint) != 0 || listen(listener, 4) != 0)
        { if (notices_) notices_("could not bind/listen on configured endpoint (WinSock " + std::to_string(WSAGetLastError()) + ")"); closesocket(listener); WSACleanup(); running_ = false; return; }
        u_long nonblocking = 1; ioctlsocket(listener, FIONBIO, &nonblocking); listening_ = true;
        SOCKET client = INVALID_SOCKET; FrameReader reader; enum class Phase { Hello, Authentication, Active }; Phase phase = Phase::Hello; ULONGLONG authDeadline = 0;

        auto closeClient = [&]
        {
            if (client != INVALID_SOCKET) { SendFrame(client, 8, {}); shutdown(client, SD_BOTH); closesocket(client); client = INVALID_SOCKET; }
            if (connected_.exchange(false)) commands_(Command{CommandKind::ReleaseAll});
            { std::lock_guard lock(outboundMutex_); outbound_.clear(); }
            phase = Phase::Hello; reader = {}; disconnect_ = false;
        };
        auto sendJson = [&](const json::Value& value) { return client != INVALID_SOCKET && SendFrame(client, 1, json::Dump(value)); };
        auto activate = [&]
        {
            phase = Phase::Active; connected_ = true; sendJson(json::Value::Object{{"type", "auth.ok"}}); sendJson(snapshot_());
        };

        while (running_)
        {
            fd_set reads; FD_ZERO(&reads); FD_SET(listener, &reads); if (client != INVALID_SOCKET) FD_SET(client, &reads);
            timeval timeout{0, 50000}; const int selected = select(0, &reads, nullptr, nullptr, &timeout);
            if (selected == SOCKET_ERROR) break;
            if (FD_ISSET(listener, &reads))
            {
                SOCKET incoming = accept(listener, nullptr, nullptr);
                if (incoming != INVALID_SOCKET)
                {
                    // A socket accepted from the nonblocking listener may itself be nonblocking.
                    // Upgrade() uses a short receive timeout and expects a blocking read; leaving
                    // FIONBIO set creates a race where recv observes WSAEWOULDBLOCK before the
                    // client's HTTP request reaches us and a valid connection is rejected.
                    u_long blocking = 0;
                    ioctlsocket(incoming, FIONBIO, &blocking);
                    if (!Upgrade(incoming)) closesocket(incoming);
                    else if (client != INVALID_SOCKET) { SendFrame(incoming, 1, json::Dump(ErrorMessage("client-busy"))); SendFrame(incoming, 8, {}); closesocket(incoming); }
                    else { client = incoming; nonblocking = 1; ioctlsocket(client, FIONBIO, &nonblocking); phase = Phase::Hello; authDeadline = GetTickCount64() + 10000; reader = {}; }
                }
            }
            if (client != INVALID_SOCKET && FD_ISSET(client, &reads))
            {
                std::array<unsigned char, 8192> buffer{}; const int count = recv(client, reinterpret_cast<char*>(buffer.data()), int(buffer.size()), 0);
                if (count <= 0) { closeClient(); continue; }
                reader.bytes.insert(reader.bytes.end(), buffer.begin(), buffer.begin() + count);
                for (;;)
                {
                    std::string payload; const int kind = reader.Next(payload); if (kind == 0) break;
                    if (kind < 0 || kind == 4) { if (kind < 0 && notices_) notices_("client sent an invalid WebSocket frame"); closeClient(); break; }
                    if (kind == 2) { if (!SendFrame(client, 10, payload)) closeClient(); continue; }
                    if (kind == 3 || kind == 5) continue;
                    json::Value root; std::string error;
                    if (!json::Parse(payload, root, error) || !root.IsObject()) { if (notices_) notices_("client sent malformed JSON"); sendJson(ErrorMessage("invalid-message")); continue; }
                    const std::string type = StringField(root, "type");
                    if (phase == Phase::Hello)
                    {
                        int64_t protocol = 0; const auto* protocolValue = root.Find("protocol");
                        if (type != "hello" || StringField(root, "client") != "thor" || !protocolValue || !protocolValue->Integer(protocol)) { sendJson(ErrorMessage("invalid-message")); closeClient(); break; }
                        if (protocol != kProtocolVersion) { sendJson(ErrorMessage("protocol-mismatch")); closeClient(); break; }
                        sendJson(json::Value::Object{{"type", "hello"}, {"protocol", kProtocolVersion}, {"game", json::Value::Object{{"version", "3.3.5a"}, {"build", 12340}}}, {"bridge", json::Value::Object{{"version", kBridgeVersion}}}});
                        if (!pairing_.Required()) activate();
                        else { phase = Phase::Authentication; sendJson(json::Value::Object{{"type", pairing_.IsPaired() ? "auth.required" : "pairing.required"}}); }
                        continue;
                    }
                    if (phase == Phase::Authentication)
                    {
                        if (type == "auth" && pairing_.Authenticate(StringField(root, "token"))) { activate(); continue; }
                        if (type == "pair.request")
                        {
                            const auto* device = root.Find("device"); std::string token;
                            if (device && pairing_.Pair(StringField(root, "code"), StringField(*device, "id"), StringField(*device, "name"), token))
                            { sendJson(json::Value::Object{{"type", "pairing.complete"}, {"token", token}}); activate(); continue; }
                        }
                        sendJson(ErrorMessage(type == "auth" ? "auth-failed" : "auth-required")); continue;
                    }
                    Command command; std::string code;
                    if (!ParseCommand(root, command, code)) { sendJson(ErrorMessage(std::move(code))); continue; }
                    if (!commands_(std::move(command))) sendJson(ErrorMessage("queue-full"));
                }
            }
            if (client != INVALID_SOCKET && phase != Phase::Active && GetTickCount64() >= authDeadline) { if (notices_) notices_("client authentication timed out"); closeClient(); }
            if (disconnect_) closeClient();
            if (client != INVALID_SOCKET && phase == Phase::Active)
            {
                std::deque<Outbound> pending; { std::lock_guard lock(outboundMutex_); pending.swap(outbound_); }
                for (const auto& message : pending) if (!SendFrame(client, 1, message.payload)) { closeClient(); break; }
            }
        }
        closeClient(); listening_ = false; closesocket(listener); WSACleanup(); running_ = false;
    }
}
