#include "ExtensionApi.hpp"
#include "SdlGamepad.hpp"

#include "engine/events/Event.hpp"
#include "game/Camera.hpp"
#include "game/Input.hpp"
#include "game/Script.hpp"
#include "game/Action.hpp"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wxl_gamepad
{
    const WXL_Api* g_api = nullptr;
    namespace
    {
        struct Stick { float x; float y; };
        constexpr float kDeadzone = .18f, kActionThreshold = .35f, kTriggerOn = .55f, kTriggerOff = .45f;
        constexpr uint32_t kTapMs = 220;
        constexpr float kTapTravel = .035f;

        Stick ReadStick(sdl::GamepadApi& api, sdl::Gamepad* pad, sdl::Axis x, sdl::Axis y)
        {
            float sx = float(api.AxisValue(pad, x)) / 32767.f, sy = float(api.AxisValue(pad, y)) / 32767.f;
            const float length = std::sqrt(sx * sx + sy * sy);
            if (length <= kDeadzone) return {};
            const float factor = (length - kDeadzone) / ((1.f - kDeadzone) * length);
            return { sx * factor, sy * factor };
        }
        void MoveAction(wxl::game::input::Control c, bool next, bool& current, uint32_t time)
        {
            if (next == current) return;
            if (next ? wxl::game::input::Begin(c, time) : wxl::game::input::End(c, time))
            { wxl::game::input::Commit(time); current = next; }
        }
        void ViewAction(wxl::offsets::engine::camera::ViewControl c, bool next, bool& current, uint32_t time)
        {
            if (next == current) return;
            if (next ? wxl::game::camera::BeginView(c, time) : wxl::game::camera::EndView(c, time)) current = next;
        }

        struct Controller;
        Controller* g_controller = nullptr;
        using ScriptFn = wxl::game::script::Function;
        using RegisterFn = void(__cdecl*)(const char*, ScriptFn);
        RegisterFn g_originalRegister = nullptr;
        wxl::game::script::ValidateCallbackFn g_originalValidate = nullptr;
        int __cdecl LuaGetBinding(void*); int __cdecl LuaSetBinding(void*); int __cdecl LuaGetState(void*);

        struct Controller final
        {
            sdl::GamepadApi sdl; sdl::Gamepad* pad = nullptr;
            bool forward = false, backward = false, turnLeft = false, turnRight = false;
            bool camLeft = false, camRight = false, camUp = false, camDown = false;
            bool activatorDown[8]{}, leftTrigger = false, rightTrigger = false;
            bool touchWasDown = false, touchMoved = false, touchPhysical = false, twoFingerCandidate = false, twoFingerMoved = false;
            uint32_t touchStart = 0; float touchStartX = 0, touchStartY = 0, touchLastX = 0, touchLastY = 0, touchX = 0, touchY = 0;
            int touchFingers = 0, activeLayer = 0, reportedLayer = -99, lastAction = 0, bindings[40]{};
            int invertCameraX = 1, invertCameraY = 1;
            bool luaRegistered = false;
            Stick move{}, camera{}; char name[128] = "no controller";

            Controller() { for (int i = 0; i < 40; ++i) bindings[i] = i + 1; }
            int Binding(int i) const { return i >= 0 && i < 40 ? bindings[i] : 0; }
            bool SetBinding(int i, int slot) { if (i < 0 || i >= 40 || slot < 1 || slot > 120) return false; bindings[i] = slot; return true; }

            void Stop(uint32_t time)
            {
                using I = wxl::game::input::Control; using V = wxl::offsets::engine::camera::ViewControl;
                MoveAction(I::Forward, false, forward, time); MoveAction(I::Backward, false, backward, time);
                MoveAction(I::TurnLeft, false, turnLeft, time); MoveAction(I::TurnRight, false, turnRight, time);
                ViewAction(V::Left, false, camLeft, time); ViewAction(V::Right, false, camRight, time);
                ViewAction(V::Up, false, camUp, time); ViewAction(V::Down, false, camDown, time);
                std::memset(activatorDown, 0, sizeof activatorDown);
                leftTrigger = rightTrigger = touchWasDown = touchPhysical = twoFingerCandidate = twoFingerMoved = false;
            }
            void RegisterLua()
            {
                if (luaRegistered || !wxl::game::script::Context()) return;
                wxl::game::script::Register("WXLGamepad_GetBinding", &LuaGetBinding);
                wxl::game::script::Register("WXLGamepad_SetBinding", &LuaSetBinding);
                wxl::game::script::Register("WXLGamepad_GetState", &LuaGetState);
                luaRegistered = true; Log(WXL_LOG_INFO, "WXLGamepad Lua configuration bridge registered");
            }
            void DispatchAction(int slot)
            {
                if (!wxl::game::action::Use(slot)) { Log(WXL_LOG_WARN, "invalid controller action slot"); return; }
                g_api->Log(WXL_LOG_INFO, kTag, "native action edge: slot=%d", slot);
                lastAction = slot;
            }
            void ClickMouse(UINT down = WM_LBUTTONDOWN, UINT up = WM_LBUTTONUP, WPARAM held = MK_LBUTTON)
            {
                HWND hwnd = GetForegroundWindow(); DWORD pid = 0; if (hwnd) GetWindowThreadProcessId(hwnd, &pid);
                if (!hwnd || pid != GetCurrentProcessId()) return;
                POINT p{}; GetCursorPos(&p); ScreenToClient(hwnd, &p); const LPARAM l = MAKELPARAM(p.x, p.y);
                SendMessageA(hwnd, down, held, l); SendMessageA(hwnd, up, 0, l);
            }
            void Touchpad(uint32_t time)
            {
                sdl::TouchFinger finger{}; const bool one = sdl.FirstTouch(pad, finger, touchFingers);
                if (touchFingers == 2)
                {
                    if (!twoFingerCandidate) { twoFingerCandidate = true; twoFingerMoved = false; touchStart = time; touchStartX = finger.x; touchStartY = finger.y; }
                    const float dx = finger.x - touchStartX, dy = finger.y - touchStartY;
                    if (dx * dx + dy * dy > kTapTravel * kTapTravel) twoFingerMoved = true;
                }
                else if (touchFingers > 2 || (twoFingerCandidate && touchFingers == 1)) twoFingerCandidate = false;
                else if (touchFingers == 0 && twoFingerCandidate)
                {
                    if (!twoFingerMoved && time - touchStart <= kTapMs) ClickMouse(WM_RBUTTONDOWN, WM_RBUTTONUP, MK_RBUTTON);
                    twoFingerCandidate = false;
                }
                if (one)
                {
                    touchX = finger.x; touchY = finger.y;
                    HWND hwnd = GetForegroundWindow(); DWORD pid = 0; if (hwnd) GetWindowThreadProcessId(hwnd, &pid);
                    if (hwnd && pid == GetCurrentProcessId() && touchWasDown)
                    {
                        // Relative trackpad: a new finger contact establishes an anchor; only
                        // subsequent deltas move the existing cursor, so repositioning never jumps.
                        RECT r{}; GetClientRect(hwnd, &r); POINT screen{}; GetCursorPos(&screen);
                        screen.x += int((finger.x - touchLastX) * (r.right - r.left) * .8f);
                        screen.y += int((finger.y - touchLastY) * (r.bottom - r.top) * .8f);
                        SetCursorPos(screen.x, screen.y); POINT p = screen; ScreenToClient(hwnd, &p);
                        SendMessageA(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(p.x, p.y));
                    }
                    if (!touchWasDown) { touchStart = time; touchStartX = finger.x; touchStartY = finger.y; touchMoved = false; }
                    const float dx = finger.x - touchStartX, dy = finger.y - touchStartY;
                    if (dx * dx + dy * dy > kTapTravel * kTapTravel) touchMoved = true;
                    touchLastX = finger.x; touchLastY = finger.y;
                }
                else if (touchWasDown && touchFingers == 0 && !touchMoved && time - touchStart <= kTapMs) ClickMouse();
                touchWasDown = one;
                const bool physical = sdl.Pressed(pad, sdl::Button::Touchpad); if (physical && !touchPhysical) ClickMouse(); touchPhysical = physical;
            }
            int Layer()
            {
                const bool l1 = sdl.Pressed(pad, sdl::Button::LeftShoulder), r1 = sdl.Pressed(pad, sdl::Button::RightShoulder);
                const float lt = float(sdl.AxisValue(pad, sdl::Axis::LeftTrigger)) / 32767.f, rt = float(sdl.AxisValue(pad, sdl::Axis::RightTrigger)) / 32767.f;
                leftTrigger = leftTrigger ? lt > kTriggerOff : lt >= kTriggerOn; rightTrigger = rightTrigger ? rt > kTriggerOff : rt >= kTriggerOn;
                const int count = int(l1) + int(leftTrigger) + int(r1) + int(rightTrigger);
                if (count != 1) return count ? -1 : 0;
                return l1 ? 1 : leftTrigger ? 2 : r1 ? 3 : 4;
            }
            void Tick(const wxl::events::UpdateArgs& update)
            {
                RegisterLua(); sdl.PumpEvents();
                if (!pad) { pad = sdl.OpenFirst(); if (!pad) return; std::snprintf(name, sizeof name, "%s", sdl.Name(pad) ? sdl.Name(pad) : "unknown SDL gamepad"); g_api->Log(WXL_LOG_INFO, kTag, "controller connected: %s", name); }
                if (!sdl.Connected(pad)) { Stop(update.timeMs); sdl.Close(pad); pad = nullptr; std::snprintf(name, sizeof name, "%s", "no controller"); Log(WXL_LOG_WARN, "controller disconnected; actions stopped"); return; }
                move = ReadStick(sdl, pad, sdl::Axis::LeftX, sdl::Axis::LeftY); camera = ReadStick(sdl, pad, sdl::Axis::RightX, sdl::Axis::RightY);
                using I = wxl::game::input::Control; using V = wxl::offsets::engine::camera::ViewControl;
                MoveAction(I::Forward, move.y < -kActionThreshold, forward, update.timeMs); MoveAction(I::Backward, move.y > kActionThreshold, backward, update.timeMs);
                MoveAction(I::TurnLeft, move.x < -kActionThreshold, turnLeft, update.timeMs); MoveAction(I::TurnRight, move.x > kActionThreshold, turnRight, update.timeMs);
                const float cx = invertCameraX ? -camera.x : camera.x, cy = invertCameraY ? -camera.y : camera.y;
                ViewAction(V::Left, cx < -kActionThreshold, camLeft, update.timeMs); ViewAction(V::Right, cx > kActionThreshold, camRight, update.timeMs);
                ViewAction(V::Up, cy < -kActionThreshold, camUp, update.timeMs); ViewAction(V::Down, cy > kActionThreshold, camDown, update.timeMs);
                activeLayer = Layer();
                if (activeLayer != reportedLayer)
                {
                    reportedLayer = activeLayer;
                    g_api->Log(WXL_LOG_INFO, kTag, "controller layer changed: %d", activeLayer + 1);
                    char script[64];
                    std::snprintf(script, sizeof script, "WXLGamepadNativeLayer=%d", activeLayer + 1);
                    wxl::game::script::Execute(script);
                }
                constexpr sdl::Button buttons[8] = { sdl::Button::South, sdl::Button::East, sdl::Button::West, sdl::Button::North, sdl::Button::DpadUp, sdl::Button::DpadDown, sdl::Button::DpadLeft, sdl::Button::DpadRight };
                for (int i = 0; i < 8; ++i) { const bool down = sdl.Pressed(pad, buttons[i]); if (down && !activatorDown[i] && activeLayer >= 0) DispatchAction(bindings[activeLayer * 8 + i]); activatorDown[i] = down; }
                Touchpad(update.timeMs);
            }
            void DrawPanel()
            {
                char text[192]; g_api->UiText("SDL3 / native action diagnostic"); g_api->UiSeparator();
                std::snprintf(text, sizeof text, "Controller: %s", pad ? name : "not connected"); g_api->UiText(text);
                std::snprintf(text, sizeof text, "Layer: %d  L2:%s R2:%s  Last action: %d", activeLayer + 1, leftTrigger ? "on" : "off", rightTrigger ? "on" : "off", lastAction); g_api->UiText(text);
                std::snprintf(text, sizeof text, "Touch: %d finger(s), %.2f %.2f", touchFingers, touchX, touchY); g_api->UiText(text);
                g_api->UiCheckbox("Invert camera horizontal", &invertCameraX); g_api->UiCheckbox("Invert camera vertical", &invertCameraY);
            }
        };
        Controller controller;

        int __cdecl LuaGetBinding(void* s) { wxl::game::script::PushNumber(s, controller.Binding(int(wxl::game::script::ToNumber(s, 1)) - 1)); return 1; }
        int __cdecl LuaSetBinding(void* s) { wxl::game::script::PushBoolean(s, controller.SetBinding(int(wxl::game::script::ToNumber(s, 1)) - 1, int(wxl::game::script::ToNumber(s, 2)))); return 1; }
        int __cdecl LuaGetState(void* s) { wxl::game::script::PushNumber(s, controller.activeLayer + 1); wxl::game::script::PushNumber(s, controller.lastAction); wxl::game::script::PushNumber(s, controller.touchX); wxl::game::script::PushNumber(s, controller.touchY); wxl::game::script::PushNumber(s, controller.touchFingers); return 5; }
        bool Ours(uintptr_t f) { return f == reinterpret_cast<uintptr_t>(&LuaGetBinding) || f == reinterpret_cast<uintptr_t>(&LuaSetBinding) || f == reinterpret_cast<uintptr_t>(&LuaGetState); }
        void __cdecl RegisterHook(const char* name, ScriptFn function) { g_originalRegister(name, function); }
        void __cdecl ValidateHook(uintptr_t f) { if (!Ours(f)) g_originalValidate(f); }
        void __cdecl OnUpdate(void*, const void* a) { controller.Tick(*static_cast<const wxl::events::UpdateArgs*>(a)); }
        void __cdecl OnWorldLeave(void*, const void*) { controller.Stop(0); }
        void __cdecl DrawPanel(void*) { controller.DrawPanel(); }
    }
    bool InstallGamepad()
    {
        if (!controller.sdl.Initialise()) return false;
        if (!g_api->HookAttachByName("Lua.RegisterFunction", reinterpret_cast<void*>(&RegisterHook), reinterpret_cast<void**>(&g_originalRegister), WXL_HOOK_DEFAULT_PRIORITY) || !g_api->HookAttachByName("Lua.ValidateFunctionPointer", reinterpret_cast<void*>(&ValidateHook), reinterpret_cast<void**>(&g_originalValidate), WXL_HOOK_DEFAULT_PRIORITY)) { Log(WXL_LOG_ERROR, "could not attach Lua configuration bridge hooks"); return false; }
        g_api->Subscribe(uint32_t(wxl::events::Event::OnUpdate), &OnUpdate, nullptr); g_api->Subscribe(uint32_t(wxl::events::Event::OnWorldLeave), &OnWorldLeave, nullptr); g_api->UiAddPanel("wxl-gamepad", &DrawPanel, nullptr);
        Log(WXL_LOG_INFO, "initialised; SDL3 controller actions and touchpad polling run on OnUpdate"); return true;
    }
}
