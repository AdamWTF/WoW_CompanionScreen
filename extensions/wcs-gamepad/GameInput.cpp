#include "GameInput.hpp"
#include "NativeSmartInteractWorld.hpp"
#include "SmartInteract.hpp"

#include "game/Input.hpp"
#include "game/Action.hpp"
#include "game/Script.hpp"
#include "game/Unit.hpp"
#include "game/World.hpp"

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace wcs_gamepad
{
    namespace
    {
        SmartInteractConfig SmartInteractSettings(const ControllerConfig& config)
        {
            SmartInteractConfig settings;
            settings.debug = config.smartInteractDebug;
            return settings;
        }
    }

    struct GameInput::SmartInteractState
    {
        NativeSmartInteractWorld world;
        SmartInteractExecutor executor;
        SmartInteractResult last{SmartInteractResult::NoTarget};

        explicit SmartInteractState(const ControllerConfig& config) : executor(world, SmartInteractSettings(config)) {}
    };

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
    GameInput::GameInput(const ControllerConfig& config) : smartInteract_(std::make_unique<SmartInteractState>(config)) {}
    GameInput::~GameInput() = default;
    void* GameInput::Window() { WindowChoice choice; EnumWindows(&Pick, reinterpret_cast<LPARAM>(&choice)); return choice.window; }
    bool GameInput::Foreground() const { HWND h = static_cast<HWND>(Window()); return h && GetForegroundWindow() == h; }
    void GameInput::WoWAction(int slot) { wxl::game::action::Use(slot); }
    void GameInput::SystemAction(CompanionSystemAction action, InputState state, uint32_t time)
    {
        if (action == CompanionSystemAction::Interact)
        {
            if (state == InputState::Pressed && smartInteract_) smartInteract_->last = smartInteract_->executor.Execute();
            return;
        }
        if (action != CompanionSystemAction::Jump) return;
        const bool changed = state == InputState::Pressed ? wxl::game::input::Begin(wxl::game::input::Control::Jump, time) : wxl::game::input::End(wxl::game::input::Control::Jump, time);
        if (changed && state == InputState::Pressed)
        {
            const unsigned long long guid = wxl::game::world::ActivePlayerGuid();
            wxl::game::unit::Jump(wxl::game::world::ResolveObject(guid, wxl::game::world::kTypeMaskPlayer), time);
        }
        if (changed) wxl::game::input::Commit(time);
    }
    const char* GameInput::LastSmartInteractResult() const
    { return smartInteract_ ? SmartInteractResultName(smartInteract_->last) : "NO_TARGET"; }
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
        case GameCommand::ToggleGameMenu: Key(VK_ESCAPE, true); Key(VK_ESCAPE, false); break;
        case GameCommand::ToggleWorldMap: wxl::game::script::Execute("ToggleFrame(WorldMapFrame)"); break;
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
        pendingPointerClick_ = true; pendingPointerRight_ = right;
    }
    void GameInput::MovePointerNormalized(float x, float y)
    {
        pendingPointerMove_ = true; pendingPointerX_ = (std::clamp)(x, 0.0f, 1.0f); pendingPointerY_ = (std::clamp)(y, 0.0f, 1.0f);
    }
    void GameInput::FlushPointerActions()
    {
        if (!pendingPointerMove_ && !pendingPointerClick_) return;
        HWND h = static_cast<HWND>(Window());
        if (!h || GetForegroundWindow() != h) { pendingPointerMove_ = pendingPointerClick_ = pendingPointerRight_ = false; return; }
        if (pendingPointerMove_)
        {
            RECT r{}; if (GetClientRect(h, &r) && r.right > r.left && r.bottom > r.top)
            {
                POINT p{r.left + int(pendingPointerX_ * float(r.right - r.left - 1)), r.top + int((1.0f - pendingPointerY_) * float(r.bottom - r.top - 1))};
                POINT screen = p; ClientToScreen(h, &screen); SetCursorPos(screen.x, screen.y); SendMessageA(h, WM_MOUSEMOVE, 0, MAKELPARAM(p.x, p.y));
            }
        }
        if (pendingPointerClick_ && (!pendingPointerRight_ || !rightMouse_))
        {
            POINT p{}; GetCursorPos(&p); ScreenToClient(h, &p); UINT down = pendingPointerRight_ ? WM_RBUTTONDOWN : WM_LBUTTONDOWN, up = pendingPointerRight_ ? WM_RBUTTONUP : WM_LBUTTONUP; WPARAM held = pendingPointerRight_ ? MK_RBUTTON : MK_LBUTTON;
            SendMessageA(h, down, held, MAKELPARAM(p.x, p.y)); SendMessageA(h, up, 0, MAKELPARAM(p.x, p.y));
        }
        pendingPointerMove_ = pendingPointerClick_ = pendingPointerRight_ = false;
    }
    void GameInput::UINavigation(UINavigationCommand command)
    {
        if (command == UINavigationCommand::Back) { Key(VK_ESCAPE, true); Key(VK_ESCAPE, false); return; }
        const char* name = nullptr;
        switch (command)
        {
        case UINavigationCommand::Up: name = "up"; break;
        case UINavigationCommand::Down: name = "down"; break;
        case UINavigationCommand::Left: name = "left"; break;
        case UINavigationCommand::Right: name = "right"; break;
        case UINavigationCommand::Confirm: name = "confirm"; break;
        case UINavigationCommand::Back: break;
        }
        if (!name) return;
        char script[128]; std::snprintf(script, sizeof script, "if WCS and WCS.UINavigation then WCS.UINavigation:Handle('%s') end", name); wxl::game::script::Execute(script);
    }
    void GameInput::ReleaseAll(uint32_t time)
    {
        for (size_t i = 0; i < 4; ++i) Movement(MovementControl(i), false, time); for (unsigned i = 0; i < 256; ++i) if (keys_[i]) Key(i, false); if (rightMouse_) MouseButton(true, false, true); cameraRemainderX_ = cameraRemainderY_ = 0; pendingPointerMove_ = pendingPointerClick_ = pendingPointerRight_ = false;
    }
}
