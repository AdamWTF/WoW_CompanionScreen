#include "Pairing.hpp"
#include "WebSocketServer.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace
{
    void Check(bool condition, const char* message) { if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
    bool SendAll(SOCKET socket, const std::string& data)
    {
        size_t offset = 0; while (offset < data.size()) { const int count = send(socket, data.data() + offset, int(data.size() - offset), 0); if (count <= 0) return false; offset += size_t(count); } return true;
    }
    SOCKET Connect(uint16_t port = 28423)
    {
        SOCKET socketValue = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); Check(socketValue != INVALID_SOCKET, "create client socket");
        sockaddr_in endpoint{}; endpoint.sin_family = AF_INET; endpoint.sin_port = htons(port); inet_pton(AF_INET, "127.0.0.1", &endpoint.sin_addr);
        Check(connect(socketValue, reinterpret_cast<sockaddr*>(&endpoint), sizeof endpoint) == 0, "connect to bridge"); return socketValue;
    }
    void Upgrade(SOCKET socketValue)
    {
        const std::string request = "GET /thor HTTP/1.1\r\nHost: 127.0.0.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
        Check(SendAll(socketValue, request), "send upgrade"); std::string response; char bytes[256]{};
        while (response.find("\r\n\r\n") == std::string::npos && response.size() < 1024)
        {
            const int count = recv(socketValue, bytes, sizeof bytes, 0); Check(count > 0, "receive upgrade response"); response.append(bytes, size_t(count));
        }
        Check(response.find("101 Switching Protocols") != std::string::npos, "upgrade accepted");
    }
    std::string MaskedFrame(std::string_view payload, unsigned char opcode = 1, bool final = true)
    {
        const unsigned char mask[4] = {0x12, 0x34, 0x56, 0x78}; std::string frame; frame.push_back(char((final ? 0x80 : 0) | opcode));
        if (payload.size() < 126) frame.push_back(char(0x80 | payload.size()));
        else { frame.push_back(static_cast<char>(0xfeu)); frame.push_back(char(payload.size() >> 8)); frame.push_back(char(payload.size())); }
        frame.append(reinterpret_cast<const char*>(mask), 4); for (size_t i = 0; i < payload.size(); ++i) frame.push_back(char(uint8_t(payload[i]) ^ mask[i & 3])); return frame;
    }
    std::string ReceiveFrame(SOCKET socketValue, unsigned char* opcodeOut = nullptr)
    {
        unsigned char header[2]{}; Check(recv(socketValue, reinterpret_cast<char*>(header), 2, MSG_WAITALL) == 2, "receive frame header");
        if (opcodeOut) *opcodeOut = header[0] & 15; uint64_t length = header[1] & 127;
        if (length == 126) { unsigned char extended[2]{}; Check(recv(socketValue, reinterpret_cast<char*>(extended), 2, MSG_WAITALL) == 2, "receive length"); length = uint64_t(extended[0]) << 8 | extended[1]; }
        Check(length <= 65536, "reasonable server payload"); std::string payload(size_t(length), '\0');
        if (length) Check(recv(socketValue, payload.data(), int(length), MSG_WAITALL) == int(length), "receive payload"); return payload;
    }
}

int main()
{
    using namespace wxl_thor;
    WSADATA winsock{}; Check(WSAStartup(MAKEWORD(2, 2), &winsock) == 0, "client Winsock startup");
    const std::string pairingPath = "build\\thor-obj\\pairing-test.dat"; DeleteFileA(pairingPath.c_str());
    PairingManager persistent(pairingPath); Check(persistent.Initialise(true), "pairing manager starts unpaired");
    const std::string code = persistent.PairingCode(); std::string token;
    Check(!code.empty() && persistent.Pair(code, "device-id", "Test device", token), "pairing code exchanges for token");
    Check(!token.empty() && persistent.Authenticate(token) && !persistent.Authenticate("wrong-token"), "token authentication");
    PairingManager reloaded(pairingPath); Check(reloaded.Initialise(true) && reloaded.IsPaired() && reloaded.Authenticate(token), "DPAPI pairing persists");
    reloaded.Forget(); Check(!reloaded.IsPaired() && !reloaded.PairingCode().empty(), "forget rotates to an unpaired code");

    PairingManager pairing; Check(pairing.Initialise(false), "pairing disabled for WebSocket test");
    WebSocketServer server(pairing, [](Command) { return true; }, [] { StateStore store; return store.SnapshotMessage(); });
    Check(server.Start("127.0.0.1", 28423), "start server thread");
    for (int i = 0; i < 100 && !server.Listening(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Check(server.Listening(), "server listening");

    SOCKET client = Connect(); Upgrade(client);
    const std::string hello = R"({"type":"hello","protocol":1,"client":"thor"})";
    const size_t half = hello.size() / 2; Check(SendAll(client, MaskedFrame(std::string_view(hello).substr(0, half), 1, false) + MaskedFrame(std::string_view(hello).substr(half), 0, true)), "send fragmented hello");
    Check(ReceiveFrame(client).find("\"type\":\"hello\"") != std::string::npos, "server hello first");
    Check(ReceiveFrame(client).find("\"type\":\"auth.ok\"") != std::string::npos, "auth ok second");
    Check(ReceiveFrame(client).find("\"type\":\"state.snapshot\"") != std::string::npos, "snapshot third");

    Check(SendAll(client, MaskedFrame("not-json")), "send malformed JSON");
    Check(ReceiveFrame(client).find("invalid-message") != std::string::npos, "malformed JSON rejected");
    Check(SendAll(client, MaskedFrame("ping", 9)), "send ping"); unsigned char opcode = 0; Check(ReceiveFrame(client, &opcode) == "ping" && opcode == 10, "pong mirrors payload");

    SOCKET second = Connect(); Upgrade(second); Check(ReceiveFrame(second).find("client-busy") != std::string::npos, "second client rejected"); closesocket(second);
    Check(SendAll(client, std::string("\x81\x02{}", 4)), "send unmasked frame"); ReceiveFrame(client, &opcode); Check(opcode == 8, "unmasked frame closes session");
    closesocket(client); server.Stop();

    PairingManager wirePairing(pairingPath); Check(wirePairing.Initialise(true), "wire pairing starts unpaired"); const std::string wireCode = wirePairing.PairingCode();
    WebSocketServer pairedServer(wirePairing, [](Command) { return true; }, [] { StateStore store; return store.SnapshotMessage(); });
    Check(pairedServer.Start("127.0.0.1", 28424), "start paired server");
    for (int i = 0; i < 100 && !pairedServer.Listening(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    SOCKET pairingClient = Connect(28424); Upgrade(pairingClient); Check(SendAll(pairingClient, MaskedFrame(hello)), "send paired hello");
    Check(ReceiveFrame(pairingClient).find("\"type\":\"hello\"") != std::string::npos, "paired server hello");
    Check(ReceiveFrame(pairingClient).find("pairing.required") != std::string::npos, "pairing requested");
    const std::string pairRequest = std::string("{\"type\":\"pair.request\",\"code\":\"") + wireCode + "\",\"device\":{\"id\":\"phone-1\",\"name\":\"Test phone\"}}";
    Check(SendAll(pairingClient, MaskedFrame(pairRequest)), "send pairing request");
    Check(ReceiveFrame(pairingClient).find("pairing.complete") != std::string::npos, "pairing completes");
    Check(ReceiveFrame(pairingClient).find("auth.ok") != std::string::npos, "paired session authenticates");
    Check(ReceiveFrame(pairingClient).find("state.snapshot") != std::string::npos, "paired session receives snapshot");
    closesocket(pairingClient); pairedServer.Stop(); wirePairing.Forget();

    WSACleanup(); std::cout << "Thor Bridge WebSocket tests passed\n"; return 0;
}
