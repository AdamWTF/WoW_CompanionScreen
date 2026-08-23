// SDL3 loaded at runtime so the extension remains a normal WarcraftXL DLL.
#pragma once

#include <cstdint>

namespace wxl_gamepad::sdl
{
    struct Gamepad;
    using JoystickId = int32_t;

    // SDL_GAMEPAD_AXIS_* and SDL_GAMEPAD_BUTTON_* values from SDL3's public API.  The SDL DLL is
    // loaded dynamically to avoid a static CRT/dependency conflict inside Wow.exe.
    enum class Axis : int { LeftX = 0, LeftY = 1, RightX = 2, RightY = 3, LeftTrigger = 4, RightTrigger = 5 };
    enum class Button : int
    {
        South = 0, East = 1, West = 2, North = 3,
        LeftShoulder = 9, RightShoulder = 10,
        DpadUp = 11, DpadDown = 12, DpadLeft = 13, DpadRight = 14,
        Touchpad = 20,
    };

    struct TouchFinger { bool down; float x; float y; float pressure; };

    class GamepadApi final
    {
    public:
        bool Initialise();
        void Shutdown();
        void PumpEvents() const;
        Gamepad* OpenFirst();
        void Close(Gamepad* gamepad);
        const char* Name(Gamepad* gamepad) const;
        const char* Error() const;
        bool Connected(Gamepad* gamepad) const;
        int16_t AxisValue(Gamepad* gamepad, Axis axis) const;
        bool Pressed(Gamepad* gamepad, Button button) const;
        bool FirstTouch(Gamepad* gamepad, TouchFinger& finger, int& fingerCount) const;

    private:
        void* module_ = nullptr;
        int (*init_)(uint32_t) = nullptr;
        void (*quitSubSystem_)(uint32_t) = nullptr;
        void (*pumpEvents_)() = nullptr;
        JoystickId* (*getGamepads_)(int*) = nullptr;
        void (*free_)(void*) = nullptr;
        Gamepad* (*openGamepad_)(JoystickId) = nullptr;
        void (*closeGamepad_)(Gamepad*) = nullptr;
        const char* (*getGamepadName_)(Gamepad*) = nullptr;
        const char* (*getError_)() = nullptr;
        uint8_t (*gamepadConnected_)(Gamepad*) = nullptr;
        int16_t (*getGamepadAxis_)(Gamepad*, int) = nullptr;
        uint8_t (*getGamepadButton_)(Gamepad*, int) = nullptr;
        int (*getNumTouchpads_)(Gamepad*) = nullptr;
        int (*getNumTouchpadFingers_)(Gamepad*, int) = nullptr;
        uint8_t (*getTouchpadFinger_)(Gamepad*, int, int, uint8_t*, float*, float*, float*) = nullptr;
    };
}
