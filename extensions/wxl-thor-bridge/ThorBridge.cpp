#include "ThorBridge.hpp"

#include "ExtensionApi.hpp"
#include "common/ExtensionConfig.hpp"
#include "game/Script.hpp"

#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace wxl_thor
{
    ThorBridge* g_bridge = nullptr;
    namespace
    {
        constexpr const char* kConfig = "Extensions\\wxl-thor-bridge\\wxl-thor-bridge.cfg";
        bool ConfigBool(const char* name, bool fallback)
        {
            char value[32]{}; return wxl::ext::config::Raw(name, value, sizeof value, kConfig) ? wxl::ext::config::Truthy(value, fallback) : fallback;
        }
        std::string ConfigString(const char* name, const char* fallback)
        {
            char value[128]{}; return wxl::ext::config::Raw(name, value, sizeof value, kConfig) ? value : fallback;
        }
        uint16_t ConfigPort()
        {
            const std::string value = ConfigString("WXL_THOR_PORT", "18423"); char* end = nullptr; const unsigned long port = std::strtoul(value.c_str(), &end, 10);
            return end && !*end && port >= 1 && port <= 65535 ? uint16_t(port) : uint16_t(18423);
        }
        json::Value EventMessage(std::string_view type, const json::Value& data) { return json::Value::Object{{"type", std::string(type)}, {"data", data}}; }
        std::string ArgString(void* state, int index)
        {
            if (!wxl::game::script::IsString(state, index)) return {}; const char* value = wxl::game::script::ToString(state, index); return value ? value : "";
        }
    }

    bool ThorBridge::Initialise()
    {
        enabled_ = ConfigBool("WXL_THOR_ENABLED", true); bindAddress_ = ConfigString("WXL_THOR_BIND_ADDRESS", "0.0.0.0"); port_ = ConfigPort();
        if (!pairing_.Initialise(ConfigBool("WXL_THOR_REQUIRE_PAIRING", true))) { Log(WXL_LOG_ERROR, "pairing manager could not initialise"); return false; }
        input_ = std::make_unique<InputDispatcher>([this](std::string code) { SendError(std::move(code)); });
        server_ = std::make_unique<WebSocketServer>(pairing_, [this](Command command) { return Enqueue(std::move(command)); }, [this] { return state_.SnapshotMessage(); },
            [](std::string notice) { if (g_api && g_api->Log) g_api->Log(WXL_LOG_WARN, kTag, "%s", notice.c_str()); });
        if (enabled_ && !server_->Start(bindAddress_, port_)) Log(WXL_LOG_ERROR, "network thread could not start; bridge remains available to WoW");
        else if (enabled_) { char line[192]; std::snprintf(line, sizeof line, "starting ws://%s:%u/thor", bindAddress_.c_str(), unsigned(port_)); Log(WXL_LOG_INFO, line); }
        else Log(WXL_LOG_INFO, "disabled by configuration; Lua/status integration remains available");
        return true;
    }

    bool ThorBridge::Enqueue(Command command)
    {
        std::lock_guard lock(commandMutex_);
        if (command.kind == CommandKind::PointerMove && !commands_.empty() && commands_.back().kind == CommandKind::PointerMove)
        {
            commands_.back().x = (std::clamp)(commands_.back().x + command.x, -32768, 32767);
            commands_.back().y = (std::clamp)(commands_.back().y + command.y, -32768, 32767); return true;
        }
        if (commands_.size() >= 256) return false; commands_.push_back(std::move(command)); return true;
    }

    void ThorBridge::OnUpdate(const wxl::events::UpdateArgs&)
    {
        void* const context = wxl::game::script::Context(); if (context && context != luaContext_) RegisterLua(context);
        std::deque<Command> pending; { std::lock_guard lock(commandMutex_); pending.swap(commands_); }
        for (const Command& command : pending) input_->Dispatch(command);
    }

    void ThorBridge::SetLifecycle(const char* state, bool clear)
    {
        if (state_.GameState() == state && !clear) return; state_.SetGameState(state, clear);
        if (server_) { server_->Send(EventMessage("game.state", json::Value::Object{{"state", state}}), true); if (clear) server_->Send(state_.SnapshotMessage(), true); }
    }

    void ThorBridge::OnWorldEnter(const wxl::events::WorldEnterArgs&) { everWorld_ = true; SetLifecycle("world", false); if (server_) server_->Send(state_.SnapshotMessage(), true); }
    void ThorBridge::OnWorldLeave(const wxl::events::WorldLeaveArgs&) { if (input_) input_->ReleaseAll(); SetLifecycle("loading", true); }
    void ThorBridge::OnLoading() { SetLifecycle("loading", false); }
    void ThorBridge::OnGlueRender() { if (everWorld_ && state_.GameState() != "world") SetLifecycle("character-select", false); }

    bool ThorBridge::PublishSnapshot(std::string_view payload, std::string& error)
    {
        if (payload.size() > kMaxMessageBytes || !ValidUtf8(payload)) { error = "snapshot is too large or not UTF-8"; return false; }
        json::Value data; if (!json::Parse(payload, data, error) || !state_.PublishSnapshot(data, error)) return false;
        if (server_) server_->Send(state_.SnapshotMessage(), true); return true;
    }

    bool ThorBridge::PublishEvent(std::string_view type, std::string_view payload, std::string& error)
    {
        if (payload.size() > kMaxMessageBytes || !ValidUtf8(payload)) { error = "event is too large or not UTF-8"; return false; }
        json::Value data; if (!json::Parse(payload, data, error) || !state_.PublishEvent(type, data, error)) return false;
        if (server_) server_->Send(EventMessage(type, data), true); return true;
    }

    std::string ThorBridge::StatusJson() const
    {
        json::Value::Object status{{"enabled", enabled_}, {"listening", server_ && server_->Listening()}, {"connected", server_ && server_->Connected()},
            {"bindAddress", bindAddress_}, {"port", int(port_)}, {"paired", pairing_.IsPaired()}, {"device", pairing_.DeviceName()}, {"pairingCode", pairing_.PairingCode()}};
        return json::Dump(status);
    }

    void ThorBridge::ForgetDevice() { pairing_.Forget(); if (server_) server_->DisconnectClient(); Log(WXL_LOG_INFO, "paired device forgotten and active session revoked"); }
    void ThorBridge::SendError(std::string code) { if (server_) server_->Send(ErrorMessage(std::move(code))); }

    void ThorBridge::DrawPanel()
    {
        g_api->UiText("Thor Bridge / WebSocket protocol v1"); g_api->UiSeparator();
        char line[256]; std::snprintf(line, sizeof line, "Endpoint: ws://%s:%u/thor", bindAddress_.c_str(), unsigned(port_)); g_api->UiText(line);
        std::snprintf(line, sizeof line, "Network: %s   Client: %s", server_ && server_->Listening() ? "listening" : "stopped", server_ && server_->Connected() ? "connected" : "none"); g_api->UiText(line);
        const std::string device = pairing_.DeviceName(), code = pairing_.PairingCode();
        if (!device.empty()) { std::snprintf(line, sizeof line, "Paired device: %s", device.c_str()); g_api->UiText(line); }
        else { std::snprintf(line, sizeof line, "Pairing code: %s", code.c_str()); g_api->UiText(line); }
        if (pairing_.IsPaired() && g_api->UiButton("Forget paired device")) ForgetDevice();
    }

    bool ThorBridge::IsOwnLuaFunction(uintptr_t function) const
    {
        return function == reinterpret_cast<uintptr_t>(&LuaPublishSnapshot) || function == reinterpret_cast<uintptr_t>(&LuaPublishEvent) ||
            function == reinterpret_cast<uintptr_t>(&LuaGetStatus) || function == reinterpret_cast<uintptr_t>(&LuaForgetDevice);
    }

    void ThorBridge::RegisterLua(void* context)
    {
        const bool replaced = luaContext_ != nullptr;
        luaContext_ = context; wxl::game::script::Register("WXLThorBridgePublishSnapshot", &LuaPublishSnapshot);
        wxl::game::script::Register("WXLThorBridgePublishEvent", &LuaPublishEvent); wxl::game::script::Register("WXLThorBridgeGetStatus", &LuaGetStatus);
        wxl::game::script::Register("WXLThorBridgeForgetDevice", &LuaForgetDevice);
        if (replaced) { state_.SetGameState(state_.GameState(), true); if (server_) server_->Send(state_.SnapshotMessage(), true); }
        Log(WXL_LOG_INFO, "registered Lua bridge in a new FrameScript context");
    }

    int __cdecl LuaPublishSnapshot(void* state)
    {
        std::string error; bool ok = false; try { ok = g_bridge && g_bridge->PublishSnapshot(ArgString(state, 1), error); } catch (...) { error = "internal bridge error"; }
        wxl::game::script::PushBoolean(state, ok); wxl::game::script::PushString(state, error.c_str()); return 2;
    }
    int __cdecl LuaPublishEvent(void* state)
    {
        std::string error; bool ok = false; try { ok = g_bridge && g_bridge->PublishEvent(ArgString(state, 1), ArgString(state, 2), error); } catch (...) { error = "internal bridge error"; }
        wxl::game::script::PushBoolean(state, ok); wxl::game::script::PushString(state, error.c_str()); return 2;
    }
    int __cdecl LuaGetStatus(void* state) { const std::string status = g_bridge ? g_bridge->StatusJson() : "{}"; wxl::game::script::PushString(state, status.c_str()); return 1; }
    int __cdecl LuaForgetDevice(void* state) { if (g_bridge) g_bridge->ForgetDevice(); wxl::game::script::PushBoolean(state, g_bridge != nullptr); return 1; }
}
