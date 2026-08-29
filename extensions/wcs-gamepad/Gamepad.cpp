#include "ExtensionApi.hpp"
#include "ControllerConfig.hpp"
#include "ControllerGameplay.hpp"
#include "ControllerManager.hpp"
#include "GameInput.hpp"

#include "engine/events/Event.hpp"
#include "game/Input.hpp"
#include "game/Script.hpp"

#include <memory>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace wcs_gamepad
{
    const WXL_Api* g_api = nullptr;
    namespace
    {
        constexpr const char* kConfigPath = "Extensions\\wcs-gamepad\\wcs-gamepad.cfg";
        std::unique_ptr<ControllerConfig> config;
        std::unique_ptr<ControllerManager> manager;
        std::unique_ptr<GameInput> input;
        std::unique_ptr<ControllerGameplay> gameplay;
        void* luaContext{};

        int __cdecl LuaResetSystemActions(void* state)
        {
            const bool ok = gameplay != nullptr; if (gameplay) gameplay->ResetSystemActions(wxl::game::input::ActionTime());
            wxl::game::script::PushBoolean(state, ok); return 1;
        }
        int __cdecl LuaSetSystemAction(void* state)
        {
            bool ok = false;
            if (gameplay && wxl::game::script::ArgCount(state) >= 3 && wxl::game::script::IsString(state, 1) && wxl::game::script::IsString(state, 2) && wxl::game::script::IsString(state, 3))
                ok = gameplay->SetSystemAction(wxl::game::script::ToString(state, 1), wxl::game::script::ToString(state, 2), wxl::game::script::ToString(state, 3));
            wxl::game::script::PushBoolean(state, ok); return 1;
        }
        int __cdecl LuaSupportsSystemAction(void* state)
        {
            const bool ok = wxl::game::script::ArgCount(state) >= 1 && wxl::game::script::IsString(state, 1) && ControllerGameplay::SupportsSystemAction(wxl::game::script::ToString(state, 1));
            wxl::game::script::PushBoolean(state, ok); return 1;
        }
        int __cdecl LuaSetUINavigationActive(void* state)
        {
            const bool active = gameplay && wxl::game::script::ArgCount(state) >= 1 && wxl::game::script::IsNumber(state, 1) && wxl::game::script::ToNumber(state, 1) != 0;
            if (gameplay) gameplay->SetUINavigationActive(active, wxl::game::input::ActionTime());
            wxl::game::script::PushBoolean(state, gameplay && gameplay->Active()); return 1;
        }
        int __cdecl LuaMovePointer(void* state)
        {
            const bool valid = input && wxl::game::script::ArgCount(state) >= 2 && wxl::game::script::IsNumber(state, 1) && wxl::game::script::IsNumber(state, 2);
            const float x = valid ? float(wxl::game::script::ToNumber(state, 1)) : 0.0f, y = valid ? float(wxl::game::script::ToNumber(state, 2)) : 0.0f;
            const bool ok = valid && std::isfinite(x) && std::isfinite(y);
            if (ok) input->MovePointerNormalized(x, y);
            wxl::game::script::PushBoolean(state, ok); return 1;
        }
        int __cdecl LuaClickPointer(void* state)
        {
            const bool valid = input && wxl::game::script::ArgCount(state) >= 1 && wxl::game::script::IsString(state, 1);
            const char* button = valid ? wxl::game::script::ToString(state, 1) : nullptr;
            const bool ok = button && (std::strcmp(button, "left") == 0 || std::strcmp(button, "right") == 0);
            if (ok) input->PointerClick(std::strcmp(button, "right") == 0);
            wxl::game::script::PushBoolean(state, ok); return 1;
        }
        void RegisterLua()
        {
            void* const context = wxl::game::script::Context(); if (!context || context == luaContext) return; luaContext = context;
            wxl::game::script::Register("WCSGamepadResetSystemActions", &LuaResetSystemActions);
            wxl::game::script::Register("WCSGamepadSetSystemAction", &LuaSetSystemAction);
            wxl::game::script::Register("WCSGamepadSupportsSystemAction", &LuaSupportsSystemAction);
            wxl::game::script::Register("WCSGamepadSetUINavigationActive", &LuaSetUINavigationActive);
            wxl::game::script::Register("WCSGamepadMovePointer", &LuaMovePointer);
            wxl::game::script::Register("WCSGamepadClickPointer", &LuaClickPointer);
            Log(WXL_LOG_INFO, "registered controller Lua API in a new FrameScript context");
        }

        void __cdecl OnUpdate(void*, const void* raw)
        {
            const auto& update = *static_cast<const wxl::events::UpdateArgs*>(raw);
            RegisterLua();
            if (manager && gameplay) gameplay->Update(manager->Snapshot(), update.dt, update.timeMs);
            if (input) input->FlushPointerActions();
        }
        void __cdecl OnWorldRenderEnd(void*, const void*)
        {
            if (gameplay && !gameplay->Active()) { gameplay->SetActive(true, 0); Log(WXL_LOG_INFO, "world rendering detected; gameplay dispatch enabled"); }
        }
        void __cdecl OnWorldLeave(void*, const void*)
        {
            if (gameplay && gameplay->Active()) { gameplay->SetActive(false, 0); Log(WXL_LOG_INFO, "world leave detected; gameplay dispatch disabled"); }
        }
        void __cdecl DrawPanel(void*)
        {
            if (!manager || !gameplay || !config) return; const ControllerSnapshot snapshot = manager->Snapshot(); char line[512];
            g_api->UiText("Normalized controller diagnostic"); g_api->UiSeparator();
            std::snprintf(line, sizeof line, "Runtime: %s   Requested: %s   Active: %s", manager->RuntimeName(), BackendName(config->backend), snapshot.backendName.c_str()); g_api->UiText(line);
            std::snprintf(line, sizeof line, "Controller: %s%s", snapshot.connected ? snapshot.device.name.c_str() : "not connected", snapshot.device.diagnosticOnly ? " (diagnostic only)" : ""); g_api->UiText(line);
            if (!snapshot.device.guid.empty()) { std::snprintf(line, sizeof line, "GUID: %s  VID:%04x PID:%04x", snapshot.device.guid.c_str(), snapshot.device.vendor, snapshot.device.product); g_api->UiText(line); }
            std::snprintf(line, sizeof line, "Left: %.2f %.2f  Right: %.2f %.2f", snapshot.state.leftX, snapshot.state.leftY, snapshot.state.rightX, snapshot.state.rightY); g_api->UiText(line);
            std::snprintf(line, sizeof line, "Gameplay: %s  Layer: %d  L2:%s R2:%s  Last action: %d", gameplay->Active() ? "in world" : "inactive", gameplay->Layer() + 1, gameplay->LeftModifier() ? "on" : "off", gameplay->RightModifier() ? "on" : "off", gameplay->LastAction()); g_api->UiText(line);
            std::snprintf(line, sizeof line, "Polling: %d Hz  Debug: %s  Glyph hint: %s", config->pollingRateHz, config->debug ? "on" : "off", snapshot.device.glyphHint.c_str()); g_api->UiText(line);
            std::snprintf(line, sizeof line, "Smart Interact: %s  Debug: %s", input->LastSmartInteractResult(), config->smartInteractDebug ? "on" : "off"); g_api->UiText(line);
        }
    }

    bool IsOwnLuaFunction(uintptr_t function)
    {
        return function == reinterpret_cast<uintptr_t>(&LuaResetSystemActions) || function == reinterpret_cast<uintptr_t>(&LuaSetSystemAction) || function == reinterpret_cast<uintptr_t>(&LuaSupportsSystemAction) ||
            function == reinterpret_cast<uintptr_t>(&LuaSetUINavigationActive) || function == reinterpret_cast<uintptr_t>(&LuaMovePointer) || function == reinterpret_cast<uintptr_t>(&LuaClickPointer);
    }

    bool InstallGamepad()
    {
        config = std::make_unique<ControllerConfig>(ControllerConfig::Load(kConfigPath)); input = std::make_unique<GameInput>(*config); gameplay = std::make_unique<ControllerGameplay>(*config, *input); manager = std::make_unique<ControllerManager>(*config);
        if (!manager->Start()) return false;
        g_api->Subscribe(uint32_t(wxl::events::Event::OnUpdate), &OnUpdate, nullptr); g_api->Subscribe(uint32_t(wxl::events::Event::OnWorldRenderEnd), &OnWorldRenderEnd, nullptr); g_api->Subscribe(uint32_t(wxl::events::Event::OnWorldLeave), &OnWorldLeave, nullptr); g_api->UiAddPanel("wcs-gamepad", &DrawPanel, nullptr);
        Log(WXL_LOG_INFO, "initialized normalized controller subsystem; gameplay dispatch is gated to an active world"); return true;
    }
}
