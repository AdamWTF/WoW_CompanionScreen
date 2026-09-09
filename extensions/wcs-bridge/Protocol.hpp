#pragma once

#include "Json.hpp"

#include <array>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace wcs_bridge
{
    inline constexpr int kProtocolVersion = 1;
    inline constexpr const char* kBridgeVersion = "1.2.0";
    inline constexpr size_t kMaxMessageBytes = 64 * 1024;

    enum class CommandKind
    {
        KeyPress, KeyDown, KeyUp, TextInsert,
        PointerMove, PointerClick, PointerDown, PointerUp, PointerScroll,
        ActionPress, PartySelect, PartyRemove, PartyPromote, PartyLeave, ReleaseAll,
    };

    struct Command
    {
        CommandKind kind{};
        std::string key;
        std::string text;
        std::string generation;
        std::string requestId;
        uint64_t session = 0;
        uint8_t modifiers = 0; // bit 0 shift, bit 1 ctrl, bit 2 alt
        int x = 0;
        int y = 0;
        int value = 0;
    };

    bool ValidUtf8(std::string_view value);
    bool ParseCommand(const json::Value& root, Command& command, std::string& errorCode);
    json::Value ErrorMessage(std::string code, std::string detail = {});

    class StateStore
    {
    public:
        StateStore();
        bool PublishSnapshot(const json::Value& data, std::string& error);
        bool PublishEvent(std::string_view type, const json::Value& data, std::string& error);
        void SetGameState(std::string state, bool clearWorldState);
        json::Value SnapshotMessage() const;
        std::string GameState() const;
        std::optional<uint64_t> PartyGuid(int member) const;
        bool ValidatePartyCommand(const Command& command, std::string& guid, std::string& error) const;

    private:
        static json::Value EmptyActions();
        static json::Value EmptyParty();
        static bool NormalizeActions(json::Value& actions, std::string& error);
        static bool NormalizeParty(json::Value& party, std::string& error);

        mutable std::mutex mutex_;
        std::string gameState_ = "login";
        json::Value player_ = nullptr;
        json::Value actions_;
        json::Value party_;
    };
}
