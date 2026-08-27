#include "GameInput.hpp"

#include "game/Input.hpp"
#include "game/Script.hpp"

#include <windows.h>
#include <algorithm>
#include <cmath>

namespace wxl_gamepad
{
    namespace
    {
        struct WindowChoice { HWND window{}; long long area{}; };
        BOOL CALLBACK Pick(HWND h, LPARAM parameter) { DWORD process{}; GetWindowThreadProcessId(h, &process); if (process != GetCurrentProcessId() || GetWindow(h, GW_OWNER) || !IsWindowVisible(h)) return TRUE; RECT r{}; GetClientRect(h, &r); long long area = long long(r.right - r.left) * (r.bottom - r.top); auto& choice = *reinterpret_cast<WindowChoice*>(parameter); if (area > choice.area) { choice.window = h; choice.area = area; } return TRUE; }
        wxl::game::input::Control NativeControl(MovementControl control)
        {
            using C = wxl::game::input::Control;
            switch (control) { case MovementControl::Forward: return C::Forward; case MovementControl::Backward: return C::Backward; case MovementControl::StrafeLeft: return C::StrafeLeft; default: return C::StrafeRight; }
        }
    }
    void* GameInput::Window() { WindowChoice choice; EnumWindows(&Pick, reinterpret_cast<LPARAM>(&choice)); return choice.window; }
    bool GameInput::Foreground() const { HWND h = static_cast<HWND>(Window()); return h && GetForegroundWindow() == h; }
    intptr_t GameInput::KeyParameter(unsigned key, bool down) { unsigned scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC); LPARAM p = 1 | LPARAM(scan << 16); if (!down) p |= LPARAM(3u << 30); return p; }
    void GameInput::Key(unsigned key, bool down)
    {
        if (!key || key >= 256 || keys_[key] == down) return; HWND h = static_cast<HWND>(Window()); if (!h) return;
        if (PostMessageA(h, down ? WM_KEYDOWN : WM_KEYUP, key, KeyParameter(key, down))) keys_[key] = down;
    }
    void GameInput::Movement(MovementControl control, bool down, uint32_t time)
    {
        const size_t index = size_t(control); if (movement_[index] == down) return;
        bool changed = down ? wxl::game::input::Begin(NativeControl(control), time) : wxl::game::input::End(NativeControl(control), time);
        if (changed) wxl::game::input::Commit(time);
        if (!down || changed) movement_[index] = down;
    }
    void GameInput::Target(const KeyChord& chord)
    {
        if (GetAsyncKeyState(int(chord.key)) & 0x8000) return; bool injected[3]{};
        for (size_t i = 0; i < chord.modifiers.size(); ++i) if (!(GetAsyncKeyState(int(chord.modifiers[i])) & 0x8000)) { Key(chord.modifiers[i], true); if (i < 3) injected[i] = true; }
        Key(chord.key, true); Key(chord.key, false); for (size_t i = chord.modifiers.size(); i-- > 0;) if (i < 3 && injected[i]) Key(chord.modifiers[i], false);
    }
    void GameInput::Command(GameCommand command)
    {
        switch (command)
        {
        case GameCommand::ToggleGameMenu: wxl::game::script::Execute("ToggleGameMenu()"); break;
        case GameCommand::ToggleAllBags: wxl::game::script::Execute("OpenAllBags()"); break;
        case GameCommand::NextView: wxl::game::script::Execute("NextView()"); break;
        }
    }
    void GameInput::MouseButton(bool right, bool down, bool force)
    {
        if (!right || rightMouse_ == down) return; HWND h = static_cast<HWND>(Window()); if (!h || (!force && GetForegroundWindow() != h)) return;
        POINT p{}; GetCursorPos(&p); ScreenToClient(h, &p); SendMessageA(h, down ? WM_RBUTTONDOWN : WM_RBUTTONUP, down ? MK_RBUTTON : 0, MAKELPARAM(p.x, p.y)); rightMouse_ = down;
    }
    void GameInput::Move(float dx, float dy, bool camera)
    {
        HWND h = static_cast<HWND>(Window()); if (!h || GetForegroundWindow() != h) return;
        cameraRemainderX_ += dx; cameraRemainderY_ += dy; int mx = int(std::trunc(cameraRemainderX_)), my = int(std::trunc(cameraRemainderY_)); cameraRemainderX_ -= mx; cameraRemainderY_ -= my; if (!mx && !my) return;
        POINT p{}; GetCursorPos(&p); p.x += mx; p.y += my; RECT r{}; GetClientRect(h, &r); POINT low{r.left, r.top}, high{r.right - 1, r.bottom - 1}; ClientToScreen(h, &low); ClientToScreen(h, &high); p.x = (std::clamp)(p.x, low.x, high.x); p.y = (std::clamp)(p.y, low.y, high.y); SetCursorPos(p.x, p.y); ScreenToClient(h, &p);
        WPARAM held = (camera && (rightMouse_ || (GetAsyncKeyState(VK_RBUTTON) & 0x8000))) ? MK_RBUTTON : 0; SendMessageA(h, WM_MOUSEMOVE, held, MAKELPARAM(p.x, p.y));
    }
    void GameInput::Camera(bool active, float dx, float dy)
    {
        const bool physical = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (active && !physical && !rightMouse_) MouseButton(true, true); if (active) Move(dx, dy, true); else if (rightMouse_) MouseButton(true, false, true);
        if (!active) cameraRemainderX_ = cameraRemainderY_ = 0;
    }
    void GameInput::PointerMove(int dx, int dy) { Move(float(dx), float(dy), false); }
    void GameInput::PointerClick(bool right)
    {
        if (right && rightMouse_) return;
        HWND h = static_cast<HWND>(Window()); if (!h || GetForegroundWindow() != h) return; POINT p{}; GetCursorPos(&p); ScreenToClient(h, &p); UINT down = right ? WM_RBUTTONDOWN : WM_LBUTTONDOWN, up = right ? WM_RBUTTONUP : WM_LBUTTONUP; WPARAM held = right ? MK_RBUTTON : MK_LBUTTON; SendMessageA(h, down, held, MAKELPARAM(p.x, p.y)); SendMessageA(h, up, 0, MAKELPARAM(p.x, p.y));
    }
    void GameInput::ReleaseAll(uint32_t time)
    {
        for (size_t i = 0; i < 4; ++i) Movement(MovementControl(i), false, time); for (unsigned i = 0; i < 256; ++i) if (keys_[i]) Key(i, false); if (rightMouse_) MouseButton(true, false, true); cameraRemainderX_ = cameraRemainderY_ = 0;
    }
}
