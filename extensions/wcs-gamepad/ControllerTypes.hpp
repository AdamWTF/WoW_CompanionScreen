#pragma once

#include <cstdint>
#include <string>

namespace wcs_gamepad
{
    enum class BackendKind { Auto, XInput, SDL, SDLJoystick };

    struct ControllerState
    {
        bool south{}, east{}, west{}, north{};
        bool dpadUp{}, dpadDown{}, dpadLeft{}, dpadRight{};
        bool leftShoulder{}, rightShoulder{};
        bool leftStickButton{}, rightStickButton{};
        bool start{}, back{};
        float leftTrigger{}, rightTrigger{};
        float leftX{}, leftY{}, rightX{}, rightY{};

        // Optional SDL-only capability retained from the original extension.
        bool touchSupported{}, touchDown{}, touchpadButton{};
        int touchFingers{};
        float touchX{}, touchY{}, touchPressure{};
    };

    struct ControllerDeviceInfo
    {
        BackendKind backend{BackendKind::Auto};
        std::string name{"no controller"};
        std::string guid;
        uint16_t vendor{}, product{};
        int index{-1}, buttonCount{}, axisCount{}, hatCount{};
        bool mapped{}, diagnosticOnly{};
        std::string glyphHint{"Xbox"};
    };

    struct ControllerSnapshot
    {
        ControllerState state{};
        ControllerDeviceInfo device{};
        uint64_t sequence{}, generation{};
        bool connected{};
        std::string backendName{"None"};
    };

    inline const char* BackendName(BackendKind kind)
    {
        switch (kind)
        {
        case BackendKind::XInput: return "XInput";
        case BackendKind::SDL: return "SDL";
        case BackendKind::SDLJoystick: return "SDLJoystick";
        default: return "Auto";
        }
    }
}
