#pragma once

#include "InputDispatcher.hpp"
#include "Pairing.hpp"
#include "Protocol.hpp"
#include "WebSocketServer.hpp"

#include "engine/events/Event.hpp"

#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace wxl_thor
{
    class ThorBridge
    {
    public:
        bool Initialise();
        void OnUpdate(const wxl::events::UpdateArgs& update);
        void OnWorldEnter(const wxl::events::WorldEnterArgs& event);
        void OnWorldLeave(const wxl::events::WorldLeaveArgs& event);
        void OnLoading();
        void OnGlueRender();
        bool PublishSnapshot(std::string_view payload, std::string& error);
        bool PublishEvent(std::string_view type, std::string_view payload, std::string& error);
        std::string StatusJson() const;
        void ForgetDevice();
        void DrawPanel();
        bool IsOwnLuaFunction(uintptr_t function) const;
        void RegisterLua(void* context);

    private:
        bool Enqueue(Command command);
        void SendError(std::string code);
        void SetLifecycle(const char* state, bool clear);

        mutable std::mutex commandMutex_;
        std::deque<Command> commands_;
        StateStore state_;
        PairingManager pairing_;
        std::unique_ptr<WebSocketServer> server_;
        std::unique_ptr<InputDispatcher> input_;
        void* luaContext_ = nullptr;
        bool enabled_ = true;
        bool everWorld_ = false;
        std::string bindAddress_ = "0.0.0.0";
        uint16_t port_ = 18423;
    };

    extern ThorBridge* g_bridge;
    int __cdecl LuaPublishSnapshot(void* state);
    int __cdecl LuaPublishEvent(void* state);
    int __cdecl LuaGetStatus(void* state);
    int __cdecl LuaForgetDevice(void* state);
}
