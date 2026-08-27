#include "SdlGamepad.hpp"
#include "ExtensionApi.hpp"
#include "Processing.hpp"

#include <windows.h>
#include <algorithm>
#include <cmath>

namespace wxl_gamepad::sdl
{
    namespace { constexpr uint32_t kInit = 0x00000200u | 0x00002000u; template<class T> bool Load(HMODULE m, const char* n, T& v) { v = reinterpret_cast<T>(GetProcAddress(m, n)); return v != nullptr; } }
    bool Api::Initialize()
    {
        if (module_) return true; module_ = LoadLibraryA("SDL3.dll"); if (!module_) { Log(WXL_LOG_WARN, "[SDL] SDL3.dll not found beside Wow.exe"); return false; } HMODULE m = static_cast<HMODULE>(module_);
#define SDL_LOAD(name, field) if (!Load(m, name, field)) { g_api->Log(WXL_LOG_ERROR, kTag, "[SDL] missing export %s", name); Shutdown(); return false; }
        SDL_LOAD("SDL_Init", init_); SDL_LOAD("SDL_QuitSubSystem", quit_); SDL_LOAD("SDL_PumpEvents", pump_); SDL_LOAD("SDL_AddGamepadMappingsFromFile", addMappings_);
        SDL_LOAD("SDL_GetGamepads", getGamepads_); SDL_LOAD("SDL_GetJoysticks", getJoysticks_); SDL_LOAD("SDL_free", free_); SDL_LOAD("SDL_IsGamepad", isGamepad_);
        SDL_LOAD("SDL_OpenGamepad", openGamepad_); SDL_LOAD("SDL_CloseGamepad", closeGamepad_); SDL_LOAD("SDL_GamepadConnected", gamepadConnected_); SDL_LOAD("SDL_GetGamepadGUIDForID", gamepadGuid_); SDL_LOAD("SDL_GetGamepadName", gamepadName_);
        SDL_LOAD("SDL_GetGamepadVendor", gamepadVendor_); SDL_LOAD("SDL_GetGamepadProduct", gamepadProduct_); SDL_LOAD("SDL_GetGamepadAxis", gamepadAxis_); SDL_LOAD("SDL_GetGamepadButton", gamepadButton_);
        SDL_LOAD("SDL_GetNumGamepadTouchpads", numTouchpads_); SDL_LOAD("SDL_GetNumGamepadTouchpadFingers", numTouchFingers_); SDL_LOAD("SDL_GetGamepadTouchpadFinger", touchFinger_);
        SDL_LOAD("SDL_OpenJoystick", openJoystick_); SDL_LOAD("SDL_CloseJoystick", closeJoystick_); SDL_LOAD("SDL_JoystickConnected", joystickConnected_); SDL_LOAD("SDL_GetJoystickName", joystickName_);
        SDL_LOAD("SDL_GetJoystickGUID", joystickGuid_); SDL_LOAD("SDL_GUIDToString", guidString_); SDL_LOAD("SDL_GetJoystickVendor", joystickVendor_); SDL_LOAD("SDL_GetJoystickProduct", joystickProduct_);
        SDL_LOAD("SDL_GetNumJoystickAxes", joystickAxes_); SDL_LOAD("SDL_GetNumJoystickButtons", joystickButtons_); SDL_LOAD("SDL_GetNumJoystickHats", joystickHats_);
        SDL_LOAD("SDL_GetJoystickAxis", joystickAxis_); SDL_LOAD("SDL_GetJoystickButton", joystickButton_); SDL_LOAD("SDL_GetJoystickHat", joystickHat_); SDL_LOAD("SDL_GetError", error_);
#undef SDL_LOAD
        if (!init_(kInit)) { g_api->Log(WXL_LOG_ERROR, kTag, "[SDL] initialization failed: %s", Error()); Shutdown(); return false; }
        Log(WXL_LOG_INFO, "[SDL] SDL3 gamepad/joystick subsystems initialized"); return true;
    }
    void Api::Shutdown() { if (!module_) return; if (quit_) quit_(kInit); FreeLibrary(static_cast<HMODULE>(module_)); module_ = nullptr; }
    void Api::Pump() const { pump_(); } void Api::AddMappings(const char* p) const { int n = addMappings_(p); g_api->Log(n >= 0 ? WXL_LOG_INFO : WXL_LOG_WARN, kTag, "[SDL] mapping database %s: %d", p, n); }
    JoystickId* Api::Gamepads(int& n) const { return getGamepads_(&n); } JoystickId* Api::Joysticks(int& n) const { return getJoysticks_(&n); } void Api::Free(void* p) const { if (p) free_(p); } bool Api::IsGamepad(JoystickId id) const { return isGamepad_(id) != 0; }
    Gamepad* Api::OpenGamepad(JoystickId id) const { return openGamepad_(id); } void Api::CloseGamepad(Gamepad* p) const { if (p) closeGamepad_(p); } bool Api::GamepadConnected(Gamepad* p) const { return p && gamepadConnected_(p) != 0; } Guid Api::GamepadGuid(JoystickId id) const { return gamepadGuid_(id); }
    const char* Api::GamepadName(Gamepad* p) const { return p ? gamepadName_(p) : nullptr; } uint16_t Api::GamepadVendor(Gamepad* p) const { return p ? gamepadVendor_(p) : 0; } uint16_t Api::GamepadProduct(Gamepad* p) const { return p ? gamepadProduct_(p) : 0; }
    int16_t Api::GamepadAxis(Gamepad* p, Axis a) const { return p ? gamepadAxis_(p, int(a)) : 0; } bool Api::GamepadButton(Gamepad* p, Button b) const { return p && gamepadButton_(p, int(b)) != 0; }
    bool Api::FirstTouch(Gamepad* p, ControllerState& s) const { if (!p || numTouchpads_(p) <= 0) return false; s.touchSupported = true; int count = numTouchFingers_(p, 0); for (int i = 0; i < count; ++i) { uint8_t down{}; float x{}, y{}, pressure{}; if (touchFinger_(p, 0, i, &down, &x, &y, &pressure) && down) { ++s.touchFingers; if (s.touchFingers == 1) { s.touchDown = true; s.touchX = x; s.touchY = y; s.touchPressure = pressure; } } } return s.touchDown; }
    Joystick* Api::OpenJoystick(JoystickId id) const { return openJoystick_(id); } void Api::CloseJoystick(Joystick* p) const { if (p) closeJoystick_(p); } bool Api::JoystickConnected(Joystick* p) const { return p && joystickConnected_(p) != 0; }
    const char* Api::JoystickName(Joystick* p) const { return p ? joystickName_(p) : nullptr; } Guid Api::JoystickGuid(Joystick* p) const { return joystickGuid_(p); } void Api::GuidString(Guid g, char* o, int n) const { guidString_(g, o, n); }
    uint16_t Api::JoystickVendor(Joystick* p) const { return joystickVendor_(p); } uint16_t Api::JoystickProduct(Joystick* p) const { return joystickProduct_(p); }
    int Api::JoystickAxes(Joystick* p) const { return joystickAxes_(p); } int Api::JoystickButtons(Joystick* p) const { return joystickButtons_(p); } int Api::JoystickHats(Joystick* p) const { return joystickHats_(p); }
    int16_t Api::JoystickAxis(Joystick* p, int i) const { return joystickAxis_(p, i); } bool Api::JoystickButton(Joystick* p, int i) const { return joystickButton_(p, i) != 0; } uint8_t Api::JoystickHat(Joystick* p, int i) const { return joystickHat_(p, i); } const char* Api::Error() const { return error_ ? error_() : "unknown SDL error"; }
}

namespace wxl_gamepad
{
    bool SDLGameControllerBackend::Initialize()
    {
        if (!api_.Initialize()) return false; api_.AddMappings("Extensions\\wxl-gamepad\\gamecontrollerdb.txt"); api_.Pump(); int count{}; auto ids = api_.Gamepads(count); g_api->Log(WXL_LOG_INFO, kTag, "[SDL] mapped gamepads detected: %d", count);
        for (int i = 0; i < count; ++i) { auto candidate = api_.OpenGamepad(ids[i]); const char* name = candidate ? api_.GamepadName(candidate) : nullptr; char guid[64]{}; api_.GuidString(api_.GamepadGuid(ids[i]), guid, sizeof guid); g_api->Log(WXL_LOG_INFO, kTag, "[SDL] Gamepad %d: %s GUID=%s VID=%04x PID=%04x", i, name ? name : "open failed", guid, candidate ? api_.GamepadVendor(candidate) : 0, candidate ? api_.GamepadProduct(candidate) : 0); if (candidate && !pad_) { pad_ = candidate; info_.backend = BackendKind::SDL; info_.index = i; info_.name = name ? name : "SDL Gamepad"; info_.guid = guid; info_.vendor = api_.GamepadVendor(candidate); info_.product = api_.GamepadProduct(candidate); info_.mapped = true; info_.glyphHint = info_.vendor == 0x054c ? "PlayStation" : "Xbox"; } else api_.CloseGamepad(candidate); }
        api_.Free(ids); return true;
    }
    void SDLGameControllerBackend::Shutdown() { api_.CloseGamepad(pad_); pad_ = nullptr; api_.Shutdown(); } bool SDLGameControllerBackend::IsControllerConnected() const { return api_.GamepadConnected(pad_); }
    bool SDLGameControllerBackend::Poll(ControllerState& s)
    {
        api_.Pump(); if (!IsControllerConnected()) return false; ControllerState n{};
        n.south = api_.GamepadButton(pad_, sdl::Button::South); n.east = api_.GamepadButton(pad_, sdl::Button::East); n.west = api_.GamepadButton(pad_, sdl::Button::West); n.north = api_.GamepadButton(pad_, sdl::Button::North);
        n.dpadUp = api_.GamepadButton(pad_, sdl::Button::DpadUp); n.dpadDown = api_.GamepadButton(pad_, sdl::Button::DpadDown); n.dpadLeft = api_.GamepadButton(pad_, sdl::Button::DpadLeft); n.dpadRight = api_.GamepadButton(pad_, sdl::Button::DpadRight);
        n.leftShoulder = api_.GamepadButton(pad_, sdl::Button::LeftShoulder); n.rightShoulder = api_.GamepadButton(pad_, sdl::Button::RightShoulder); n.leftStickButton = api_.GamepadButton(pad_, sdl::Button::LeftStick); n.rightStickButton = api_.GamepadButton(pad_, sdl::Button::RightStick);
        n.start = api_.GamepadButton(pad_, sdl::Button::Start); n.back = api_.GamepadButton(pad_, sdl::Button::Back); n.touchpadButton = api_.GamepadButton(pad_, sdl::Button::Touchpad);
        n.leftX = NormalizeSigned16(api_.GamepadAxis(pad_, sdl::Axis::LeftX)); n.leftY = NormalizeSigned16(api_.GamepadAxis(pad_, sdl::Axis::LeftY)); n.rightX = NormalizeSigned16(api_.GamepadAxis(pad_, sdl::Axis::RightX)); n.rightY = NormalizeSigned16(api_.GamepadAxis(pad_, sdl::Axis::RightY));
        n.leftTrigger = (std::max)(0.0f, NormalizeSigned16(api_.GamepadAxis(pad_, sdl::Axis::LeftTrigger))); n.rightTrigger = (std::max)(0.0f, NormalizeSigned16(api_.GamepadAxis(pad_, sdl::Axis::RightTrigger))); api_.FirstTouch(pad_, n); s = n; return true;
    }
    bool SDLJoystickBackend::Initialize()
    {
        if (!api_.Initialize()) return false; api_.AddMappings("Extensions\\wxl-gamepad\\gamecontrollerdb.txt"); api_.Pump(); int count{}; auto ids = api_.Joysticks(count); g_api->Log(WXL_LOG_INFO, kTag, "[SDL] Joysticks detected: %d", count);
        for (int i = 0; i < count; ++i) { auto candidate = api_.OpenJoystick(ids[i]); if (!candidate) continue; char guid[64]{}; api_.GuidString(api_.JoystickGuid(candidate), guid, sizeof guid); bool mapped = api_.IsGamepad(ids[i]); const char* name = api_.JoystickName(candidate); g_api->Log(WXL_LOG_INFO, kTag, "[SDL] Device %d: Name=%s GUID=%s VID=%04x PID=%04x GameController=%s Buttons=%d Axes=%d Hats=%d", i, name ? name : "unknown", guid, api_.JoystickVendor(candidate), api_.JoystickProduct(candidate), mapped ? "yes" : "no", api_.JoystickButtons(candidate), api_.JoystickAxes(candidate), api_.JoystickHats(candidate)); if (!mapped && !joystick_) { joystick_ = candidate; info_.backend = BackendKind::SDLJoystick; info_.index = i; info_.name = name ? name : "SDL Joystick"; info_.guid = guid; info_.vendor = api_.JoystickVendor(candidate); info_.product = api_.JoystickProduct(candidate); info_.buttonCount = api_.JoystickButtons(candidate); info_.axisCount = api_.JoystickAxes(candidate); info_.hatCount = api_.JoystickHats(candidate); info_.diagnosticOnly = true; } else api_.CloseJoystick(candidate); }
        api_.Free(ids); if (joystick_) { axes_.resize(info_.axisCount); buttons_.resize(info_.buttonCount); hats_.resize(info_.hatCount); } return true;
    }
    void SDLJoystickBackend::Shutdown() { api_.CloseJoystick(joystick_); joystick_ = nullptr; api_.Shutdown(); axes_.clear(); buttons_.clear(); hats_.clear(); } bool SDLJoystickBackend::IsControllerConnected() const { return api_.JoystickConnected(joystick_); }
    bool SDLJoystickBackend::Poll(ControllerState& s)
    {
        api_.Pump(); if (!IsControllerConnected()) return false; s = {}; uint32_t now = GetTickCount();
        for (int i = 0; i < info_.buttonCount; ++i) { uint8_t v = api_.JoystickButton(joystick_, i); if (debug_ && v != buttons_[i]) g_api->Log(WXL_LOG_INFO, kTag, "[SDLJoystick] Button %d %s", i, v ? "DOWN" : "UP"); buttons_[i] = v; }
        for (int i = 0; i < info_.hatCount; ++i) { uint8_t v = api_.JoystickHat(joystick_, i); if (debug_ && v != hats_[i]) g_api->Log(WXL_LOG_INFO, kTag, "[SDLJoystick] Hat %d = 0x%02x", i, v); hats_[i] = v; }
        for (int i = 0; i < info_.axisCount; ++i) { int16_t v = api_.JoystickAxis(joystick_, i); if (debug_ && std::abs(int(v) - int(axes_[i])) >= 2048 && now - lastAxisLog_ >= 100) { g_api->Log(WXL_LOG_INFO, kTag, "[SDLJoystick] Axis %d = %d", i, int(v)); lastAxisLog_ = now; } axes_[i] = v; }
        return true;
    }
}
