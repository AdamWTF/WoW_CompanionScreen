#pragma once

#include "ControllerTypes.hpp"

namespace wxl_gamepad
{
    class IControllerBackend
    {
    public:
        virtual ~IControllerBackend() = default;
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual bool IsControllerConnected() const = 0;
        virtual bool Poll(ControllerState& state) = 0;
        virtual ControllerDeviceInfo GetDeviceInfo() const = 0;
        virtual const char* GetBackendName() const = 0;
    };
}
