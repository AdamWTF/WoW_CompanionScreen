#pragma once

#include "Pairing.hpp"
#include "Protocol.hpp"

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace wcs_bridge
{
    class WebSocketServer
    {
    public:
        using CommandSink = std::function<bool(Command)>;
        using SnapshotSource = std::function<json::Value()>;
        using NoticeSink = std::function<void(std::string)>;

        WebSocketServer(PairingManager& pairing, CommandSink commands, SnapshotSource snapshot, NoticeSink notices = {});
        ~WebSocketServer();
        bool Start(std::string address, uint16_t port);
        void Stop();
        bool Send(const json::Value& message, bool replaceable = false);
        void DisconnectClient();
        bool Listening() const { return listening_.load(); }
        bool Connected() const { return connected_.load(); }
        bool PartyCapable() const { return partyCapable_.load(); }
        uint64_t Session() const { return session_.load(); }
        std::string Address() const;
        uint16_t Port() const { return port_; }

    private:
        struct Outbound { std::string payload; bool replaceable = false; };
        void Run();
        void RunImpl();

        PairingManager& pairing_;
        CommandSink commands_;
        SnapshotSource snapshot_;
        NoticeSink notices_;
        std::string address_;
        uint16_t port_ = 0;
        std::atomic<bool> running_{false}, listening_{false}, connected_{false}, disconnect_{false}, partyCapable_{false};
        std::thread thread_;
        std::atomic<uint64_t> session_{0};
        mutable std::mutex outboundMutex_;
        std::deque<Outbound> outbound_;
    };
}
