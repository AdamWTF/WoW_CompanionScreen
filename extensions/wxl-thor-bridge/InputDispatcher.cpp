#include "InputDispatcher.hpp"

#include "game/Action.hpp"

#include <windows.h>
#include <algorithm>
#include <map>
#include <vector>

namespace wxl_thor
{
    namespace
    {
        struct WindowChoice { HWND hwnd = nullptr; long long area = 0; };
        BOOL CALLBACK FindWindowProc(HWND hwnd, LPARAM parameter)
        {
            DWORD process = 0; GetWindowThreadProcessId(hwnd, &process);
            if (process != GetCurrentProcessId() || GetWindow(hwnd, GW_OWNER) || !IsWindowVisible(hwnd)) return TRUE;
            RECT rect{}; GetClientRect(hwnd, &rect); const long long area = long long(rect.right - rect.left) * (rect.bottom - rect.top);
            auto& choice = *reinterpret_cast<WindowChoice*>(parameter); if (area > choice.area) { choice.hwnd = hwnd; choice.area = area; } return TRUE;
        }

        LPARAM KeyLParam(unsigned key, bool down)
        {
            const unsigned scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC); LPARAM value = 1 | LPARAM(scan << 16);
            if (key == VK_RMENU || key == VK_RCONTROL || key == VK_INSERT || key == VK_DELETE || key == VK_HOME || key == VK_END || key == VK_PRIOR || key == VK_NEXT || key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN) value |= LPARAM(1u << 24);
            if (!down) value |= LPARAM(3u << 30); return value;
        }

        UINT DownMessage(int button) { return button == 0 ? WM_LBUTTONDOWN : button == 1 ? WM_RBUTTONDOWN : WM_MBUTTONDOWN; }
        UINT UpMessage(int button) { return button == 0 ? WM_LBUTTONUP : button == 1 ? WM_RBUTTONUP : WM_MBUTTONUP; }
        WPARAM ButtonMask(int button) { return button == 0 ? MK_LBUTTON : button == 1 ? MK_RBUTTON : MK_MBUTTON; }
    }

    void* InputDispatcher::WowWindow() { WindowChoice choice; EnumWindows(&FindWindowProc, reinterpret_cast<LPARAM>(&choice)); return choice.hwnd; }

    unsigned InputDispatcher::VirtualKey(const std::string& key)
    {
        if (key.size() == 1)
        {
            const SHORT mapped = VkKeyScanA(key[0]); return mapped == -1 ? 0 : LOBYTE(mapped);
        }
        static const std::map<std::string, unsigned, std::less<>> names = {
            {"BACKSPACE", VK_BACK}, {"TAB", VK_TAB}, {"ENTER", VK_RETURN}, {"ESCAPE", VK_ESCAPE}, {"SPACE", VK_SPACE},
            {"PAGEUP", VK_PRIOR}, {"PAGEDOWN", VK_NEXT}, {"END", VK_END}, {"HOME", VK_HOME}, {"LEFT", VK_LEFT},
            {"UP", VK_UP}, {"RIGHT", VK_RIGHT}, {"DOWN", VK_DOWN}, {"INSERT", VK_INSERT}, {"DELETE", VK_DELETE},
            {"SHIFT", VK_SHIFT}, {"CTRL", VK_CONTROL}, {"ALT", VK_MENU},
            {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4}, {"F5", VK_F5}, {"F6", VK_F6},
            {"F7", VK_F7}, {"F8", VK_F8}, {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},
        };
        const auto it = names.find(key); return it == names.end() ? 0 : it->second;
    }

    void InputDispatcher::Key(unsigned key, bool down)
    {
        if (!key || key >= keys_.size() || keys_[key] == down) return;
        HWND hwnd = static_cast<HWND>(WowWindow()); if (!hwnd) { errors_("game-window-unavailable"); return; }
        if (PostMessageA(hwnd, down ? WM_KEYDOWN : WM_KEYUP, key, KeyLParam(key, down))) keys_[key] = down;
    }

    void InputDispatcher::Modifiers(uint8_t modifiers, bool down)
    {
        static constexpr unsigned keys[] = {VK_SHIFT, VK_CONTROL, VK_MENU};
        if (down)
        {
            for (int i = 0; i < 3; ++i) if (modifiers & (1u << i)) Key(keys[i], true);
        }
        else
        {
            for (int i = 2; i >= 0; --i) if (modifiers & (1u << i)) Key(keys[i], false);
        }
    }

    bool InputDispatcher::PointerReady(void*& window)
    {
        HWND hwnd = static_cast<HWND>(WowWindow()); window = hwnd;
        if (!hwnd || GetForegroundWindow() != hwnd) { errors_("game-not-foreground"); return false; }
        return true;
    }

    void InputDispatcher::PointerButton(int button, bool down, bool force)
    {
        if (button < 0 || button >= int(buttons_.size()) || buttons_[size_t(button)] == down) return;
        void* raw = nullptr; if (!force && !PointerReady(raw)) return; HWND hwnd = force ? static_cast<HWND>(WowWindow()) : static_cast<HWND>(raw); if (!hwnd) return;
        POINT point{}; GetCursorPos(&point); ScreenToClient(hwnd, &point);
        WPARAM held = 0; for (int i = 0; i < 3; ++i) if (buttons_[size_t(i)] || (i == button && down)) held |= ButtonMask(i);
        SendMessageA(hwnd, down ? DownMessage(button) : UpMessage(button), down ? held : held & ~ButtonMask(button), MAKELPARAM(point.x, point.y)); buttons_[size_t(button)] = down;
    }

    void InputDispatcher::Dispatch(const Command& command)
    {
        if (command.kind == CommandKind::ReleaseAll) { ReleaseAll(); return; }
        if (command.kind == CommandKind::ActionPress) { wxl::game::action::Use(command.value + 24); return; }
        if (command.kind == CommandKind::KeyPress || command.kind == CommandKind::KeyDown || command.kind == CommandKind::KeyUp)
        {
            const unsigned key = VirtualKey(command.key); if (!key) { errors_("invalid-message"); return; }
            if (command.kind == CommandKind::KeyPress) { Modifiers(command.modifiers, true); Key(key, true); Key(key, false); Modifiers(command.modifiers, false); }
            else if (command.kind == CommandKind::KeyDown) { Modifiers(command.modifiers, true); Key(key, true); }
            else { Key(key, false); Modifiers(command.modifiers, false); }
            return;
        }
        if (command.kind == CommandKind::TextInsert)
        {
            HWND hwnd = static_cast<HWND>(WowWindow()); if (!hwnd) { errors_("game-window-unavailable"); return; }
            const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.text.data(), int(command.text.size()), nullptr, 0);
            if (count <= 0) { errors_("invalid-message"); return; }
            std::vector<wchar_t> chars(static_cast<size_t>(count), wchar_t{}); MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.text.data(), int(command.text.size()), chars.data(), count);
            for (wchar_t c : chars) PostMessageW(hwnd, WM_CHAR, WPARAM(c), 1); return;
        }
        void* raw = nullptr; if (!PointerReady(raw)) return; HWND hwnd = static_cast<HWND>(raw);
        if (command.kind == CommandKind::PointerMove)
        {
            POINT point{}; GetCursorPos(&point); point.x += command.x; point.y += command.y;
            RECT rect{}; GetClientRect(hwnd, &rect); POINT min{rect.left, rect.top}, max{rect.right - 1, rect.bottom - 1}; ClientToScreen(hwnd, &min); ClientToScreen(hwnd, &max);
            point.x = (std::clamp)(point.x, min.x, max.x); point.y = (std::clamp)(point.y, min.y, max.y); SetCursorPos(point.x, point.y);
            ScreenToClient(hwnd, &point); WPARAM held = 0; for (int i = 0; i < 3; ++i) if (buttons_[size_t(i)]) held |= ButtonMask(i);
            SendMessageA(hwnd, WM_MOUSEMOVE, held, MAKELPARAM(point.x, point.y)); return;
        }
        if (command.kind == CommandKind::PointerScroll)
        {
            POINT point{}; GetCursorPos(&point); WPARAM keys = 0; for (int i = 0; i < 3; ++i) if (buttons_[size_t(i)]) keys |= ButtonMask(i);
            SendMessageA(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(keys, SHORT(command.value * WHEEL_DELTA)), MAKELPARAM(point.x, point.y)); return;
        }
        if (command.kind == CommandKind::PointerClick) { PointerButton(command.value, true); PointerButton(command.value, false); }
        else PointerButton(command.value, command.kind == CommandKind::PointerDown);
    }

    void InputDispatcher::ReleaseAll()
    {
        for (unsigned key = 0; key < keys_.size(); ++key) if (keys_[key]) Key(key, false);
        for (int button = 0; button < 3; ++button) if (buttons_[size_t(button)]) PointerButton(button, false, true);
    }
}
