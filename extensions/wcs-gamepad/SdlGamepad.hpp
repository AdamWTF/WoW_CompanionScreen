#pragma once

#include "IControllerBackend.hpp"
#include <cstdint>
#include <vector>

namespace wcs_gamepad::sdl
{
    struct Gamepad; struct Joystick; using JoystickId = uint32_t; struct Guid { uint8_t data[16]; };
    enum class Axis : int { LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger };
    enum class Button : int { South, East, West, North, Back, Guide, Start, LeftStick, RightStick, LeftShoulder, RightShoulder, DpadUp, DpadDown, DpadLeft, DpadRight, Misc1, RightPaddle1, LeftPaddle1, RightPaddle2, LeftPaddle2, Touchpad };

    class Api final
    {
    public:
        bool Initialize(); void Shutdown(); void Pump() const; void AddMappings(const char* path) const;
        JoystickId* Gamepads(int& count) const; JoystickId* Joysticks(int& count) const; void Free(void*) const; bool IsGamepad(JoystickId) const;
        Gamepad* OpenGamepad(JoystickId) const; void CloseGamepad(Gamepad*) const; bool GamepadConnected(Gamepad*) const; Guid GamepadGuid(JoystickId) const;
        const char* GamepadName(Gamepad*) const; uint16_t GamepadVendor(Gamepad*) const; uint16_t GamepadProduct(Gamepad*) const;
        int16_t GamepadAxis(Gamepad*, Axis) const; bool GamepadButton(Gamepad*, Button) const; bool FirstTouch(Gamepad*, ControllerState&) const;
        Joystick* OpenJoystick(JoystickId) const; void CloseJoystick(Joystick*) const; bool JoystickConnected(Joystick*) const;
        const char* JoystickName(Joystick*) const; Guid JoystickGuid(Joystick*) const; void GuidString(Guid, char*, int) const;
        uint16_t JoystickVendor(Joystick*) const; uint16_t JoystickProduct(Joystick*) const;
        int JoystickAxes(Joystick*) const; int JoystickButtons(Joystick*) const; int JoystickHats(Joystick*) const;
        int16_t JoystickAxis(Joystick*, int) const; bool JoystickButton(Joystick*, int) const; uint8_t JoystickHat(Joystick*, int) const; const char* Error() const;
    private:
        void* module_{}; uint8_t (*init_)(uint32_t){}; void (*quit_)(uint32_t){}; void (*pump_)(){}; int (*addMappings_)(const char*){};
        JoystickId* (*getGamepads_)(int*){}; JoystickId* (*getJoysticks_)(int*){}; void (*free_)(void*){}; uint8_t (*isGamepad_)(JoystickId){};
        Gamepad* (*openGamepad_)(JoystickId){}; void (*closeGamepad_)(Gamepad*){}; uint8_t (*gamepadConnected_)(Gamepad*){}; Guid (*gamepadGuid_)(JoystickId){};
        const char* (*gamepadName_)(Gamepad*){}; uint16_t (*gamepadVendor_)(Gamepad*){}; uint16_t (*gamepadProduct_)(Gamepad*){};
        int16_t (*gamepadAxis_)(Gamepad*, int){}; uint8_t (*gamepadButton_)(Gamepad*, int){};
        int (*numTouchpads_)(Gamepad*){}; int (*numTouchFingers_)(Gamepad*, int){}; uint8_t (*touchFinger_)(Gamepad*, int, int, uint8_t*, float*, float*, float*){};
        Joystick* (*openJoystick_)(JoystickId){}; void (*closeJoystick_)(Joystick*){}; uint8_t (*joystickConnected_)(Joystick*){};
        const char* (*joystickName_)(Joystick*){}; Guid (*joystickGuid_)(Joystick*){}; void (*guidString_)(Guid, char*, int){};
        uint16_t (*joystickVendor_)(Joystick*){}; uint16_t (*joystickProduct_)(Joystick*){}; int (*joystickAxes_)(Joystick*){}; int (*joystickButtons_)(Joystick*){}; int (*joystickHats_)(Joystick*){};
        int16_t (*joystickAxis_)(Joystick*, int){}; uint8_t (*joystickButton_)(Joystick*, int){}; uint8_t (*joystickHat_)(Joystick*, int){}; const char* (*error_)(){};
    };
}

namespace wcs_gamepad
{
    class SDLGameControllerBackend final : public IControllerBackend
    {
    public:
        bool Initialize() override; void Shutdown() override; bool IsControllerConnected() const override; bool Poll(ControllerState&) override;
        ControllerDeviceInfo GetDeviceInfo() const override { return info_; } const char* GetBackendName() const override { return "SDL Gamepad"; }
    private: sdl::Api api_; sdl::Gamepad* pad_{}; ControllerDeviceInfo info_{};
    };
    class SDLJoystickBackend final : public IControllerBackend
    {
    public:
        explicit SDLJoystickBackend(bool debug) : debug_(debug) {}
        bool Initialize() override; void Shutdown() override; bool IsControllerConnected() const override; bool Poll(ControllerState&) override;
        ControllerDeviceInfo GetDeviceInfo() const override { return info_; } const char* GetBackendName() const override { return "SDL Joystick"; }
    private:
        sdl::Api api_; sdl::Joystick* joystick_{}; ControllerDeviceInfo info_{}; bool debug_{};
        std::vector<int16_t> axes_; std::vector<uint8_t> buttons_, hats_; uint32_t lastAxisLog_{};
    };
}
