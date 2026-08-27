#include "ExtensionApi.hpp"
#include "ControllerConfig.hpp"
#include "ControllerGameplay.hpp"
#include "ControllerManager.hpp"
#include "GameInput.hpp"

#include "engine/events/Event.hpp"

#include <memory>
#include <cstdio>

namespace wxl_gamepad
{
    const WXL_Api* g_api = nullptr;
    namespace
    {
        constexpr const char* kConfigPath = "Extensions\\wxl-gamepad\\wxl-gamepad.cfg";
        std::unique_ptr<ControllerConfig> config;
        std::unique_ptr<ControllerManager> manager;
        std::unique_ptr<GameInput> input;
        std::unique_ptr<ControllerGameplay> gameplay;

        void __cdecl OnUpdate(void*, const void* raw)
        {
            const auto& update = *static_cast<const wxl::events::UpdateArgs*>(raw);
            if (manager && gameplay) gameplay->Update(manager->Snapshot(), update.dt, update.timeMs);
        }
        void __cdecl OnWorldLeave(void*, const void*) { if (gameplay) gameplay->Release(0); }
        void __cdecl DrawPanel(void*)
        {
            if (!manager || !gameplay || !config) return; const ControllerSnapshot snapshot = manager->Snapshot(); char line[512];
            g_api->UiText("Normalized controller diagnostic"); g_api->UiSeparator();
            std::snprintf(line, sizeof line, "Runtime: %s   Requested: %s   Active: %s", manager->RuntimeName(), BackendName(config->backend), snapshot.backendName.c_str()); g_api->UiText(line);
            std::snprintf(line, sizeof line, "Controller: %s%s", snapshot.connected ? snapshot.device.name.c_str() : "not connected", snapshot.device.diagnosticOnly ? " (diagnostic only)" : ""); g_api->UiText(line);
            if (!snapshot.device.guid.empty()) { std::snprintf(line, sizeof line, "GUID: %s  VID:%04x PID:%04x", snapshot.device.guid.c_str(), snapshot.device.vendor, snapshot.device.product); g_api->UiText(line); }
            std::snprintf(line, sizeof line, "Left: %.2f %.2f  Right: %.2f %.2f", snapshot.state.leftX, snapshot.state.leftY, snapshot.state.rightX, snapshot.state.rightY); g_api->UiText(line);
            std::snprintf(line, sizeof line, "Layer: %d  L2:%s R2:%s  Last action: %d", gameplay->Layer() + 1, gameplay->LeftModifier() ? "on" : "off", gameplay->RightModifier() ? "on" : "off", gameplay->LastAction()); g_api->UiText(line);
            std::snprintf(line, sizeof line, "Polling: %d Hz  Debug: %s  Glyph hint: %s", config->pollingRateHz, config->debug ? "on" : "off", snapshot.device.glyphHint.c_str()); g_api->UiText(line);
        }
    }

    bool InstallGamepad()
    {
        config = std::make_unique<ControllerConfig>(ControllerConfig::Load(kConfigPath)); input = std::make_unique<GameInput>(); gameplay = std::make_unique<ControllerGameplay>(*config, *input); manager = std::make_unique<ControllerManager>(*config);
        if (!manager->Start()) return false;
        g_api->Subscribe(uint32_t(wxl::events::Event::OnUpdate), &OnUpdate, nullptr); g_api->Subscribe(uint32_t(wxl::events::Event::OnWorldLeave), &OnWorldLeave, nullptr); g_api->UiAddPanel("wxl-gamepad", &DrawPanel, nullptr);
        Log(WXL_LOG_INFO, "initialized normalized controller subsystem; gameplay dispatch runs on OnUpdate"); return true;
    }
}
