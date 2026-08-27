#pragma once

#include "IControllerBackend.hpp"
#include <windows.h>

namespace wxl_gamepad
{
    class XInputControllerBackend final : public IControllerBackend
    {
    public:
        bool Initialize() override;
        void Shutdown() override;
        bool IsControllerConnected() const override { return connected_; }
        bool Poll(ControllerState& state) override;
        ControllerDeviceInfo GetDeviceInfo() const override { return info_; }
        const char* GetBackendName() const override { return "XInput"; }
    private:
        struct Gamepad { unsigned short buttons; unsigned char leftTrigger, rightTrigger; short leftX, leftY, rightX, rightY; };
        struct State { unsigned long packet; Gamepad gamepad; };
        using GetStateFn = unsigned long(WINAPI*)(unsigned long, State*);
        HMODULE module_{}; GetStateFn getState_{}; unsigned slot_{}; bool connected_{}; ControllerDeviceInfo info_{};
    };
}
