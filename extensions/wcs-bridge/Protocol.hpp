#pragma once

#include "Json.hpp"

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

namespace wcs_bridge
{
    inline constexpr int kProtocolVersion = 1;
    inline constexpr const char* kBridgeVersion = "1.0.2";
    inline constexpr size_t kMaxMessageBytes = 64 * 1024;

    enum class CommandKind
    {
        KeyPress, KeyDown, KeyUp, TextInsert,
        PointerMove, PointerClick, PointerDown, PointerUp, PointerScroll,
        ActionPress, ReleaseAll,
    };

    struct Command
    {
        CommandKind kind{};
        std::string key;
        std::string text;
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

    private:
        static json::Value EmptyActions();
        static bool NormalizeActions(json::Value& actions, std::string& error);

        mutable std::mutex mutex_;
        std::string gameState_ = "login";
        json::Value player_ = nullptr;
        json::Value actions_;
    };
}
