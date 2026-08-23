#include "SdlGamepad.hpp"
#include "ExtensionApi.hpp"

#include <windows.h>

namespace wxl_gamepad::sdl
{
    namespace
    {
        constexpr uint32_t kInitGamepad = 0x00002000u; // SDL_INIT_GAMEPAD

        template <class T> bool Resolve(HMODULE module, const char* name, T& out)
        {
            out = reinterpret_cast<T>(GetProcAddress(module, name));
            return out != nullptr;
        }
    }

    bool GamepadApi::Initialise()
    {
        if (module_) return true;
        module_ = LoadLibraryA("SDL3.dll");
        if (!module_)
        {
            Log(WXL_LOG_ERROR, "SDL3.dll was not found beside Wow.exe; gamepad support is inactive");
            return false;
        }
        const HMODULE module = static_cast<HMODULE>(module_);
        if (!Resolve(module, "SDL_Init", init_) || !Resolve(module, "SDL_QuitSubSystem", quitSubSystem_) ||
            !Resolve(module, "SDL_PumpEvents", pumpEvents_) ||
            !Resolve(module, "SDL_GetGamepads", getGamepads_) || !Resolve(module, "SDL_free", free_) ||
            !Resolve(module, "SDL_OpenGamepad", openGamepad_) || !Resolve(module, "SDL_CloseGamepad", closeGamepad_) ||
            !Resolve(module, "SDL_GetGamepadName", getGamepadName_) ||
            !Resolve(module, "SDL_GetError", getError_) ||
            !Resolve(module, "SDL_GamepadConnected", gamepadConnected_) ||
            !Resolve(module, "SDL_GetGamepadAxis", getGamepadAxis_) ||
            !Resolve(module, "SDL_GetGamepadButton", getGamepadButton_) ||
            !Resolve(module, "SDL_GetNumGamepadTouchpads", getNumTouchpads_) ||
            !Resolve(module, "SDL_GetNumGamepadTouchpadFingers", getNumTouchpadFingers_) ||
            !Resolve(module, "SDL_GetGamepadTouchpadFinger", getTouchpadFinger_))
        {
            Log(WXL_LOG_ERROR, "SDL3.dll is missing required SDL3 gamepad exports");
            FreeLibrary(module);
            module_ = nullptr;
            return false;
        }
        // SDL3 returns true on success (unlike SDL2's 0-success convention).
        if (init_(kInitGamepad) == 0)
        {
            if (g_api && getError_)
                g_api->Log(WXL_LOG_ERROR, kTag, "SDL gamepad subsystem initialisation failed: %s", getError_());
            else
                Log(WXL_LOG_ERROR, "SDL gamepad subsystem initialisation failed");
            FreeLibrary(module);
            module_ = nullptr;
            return false;
        }
        Log(WXL_LOG_INFO, "SDL3 gamepad subsystem initialised");
        return true;
    }

    void GamepadApi::Shutdown()
    {
        if (!module_) return;
        quitSubSystem_(kInitGamepad);
        FreeLibrary(static_cast<HMODULE>(module_));
        module_ = nullptr;
    }

    void GamepadApi::PumpEvents() const { if (pumpEvents_) pumpEvents_(); }

    Gamepad* GamepadApi::OpenFirst()
    {
        if (!module_) return nullptr;
        int count = 0;
        JoystickId* const ids = getGamepads_(&count);
        if (!ids || count <= 0) { if (ids) free_(ids); return nullptr; }
        Gamepad* const gamepad = openGamepad_(ids[0]);
        free_(ids);
        return gamepad;
    }

    void GamepadApi::Close(Gamepad* gamepad) { if (gamepad && closeGamepad_) closeGamepad_(gamepad); }
    const char* GamepadApi::Name(Gamepad* gamepad) const { return gamepad ? getGamepadName_(gamepad) : nullptr; }
    const char* GamepadApi::Error() const { return getError_ ? getError_() : "SDL error unavailable"; }
    bool GamepadApi::Connected(Gamepad* gamepad) const { return gamepad && gamepadConnected_(gamepad) != 0; }
    int16_t GamepadApi::AxisValue(Gamepad* gamepad, Axis axis) const { return gamepad ? getGamepadAxis_(gamepad, int(axis)) : 0; }
    bool GamepadApi::Pressed(Gamepad* gamepad, Button button) const { return gamepad && getGamepadButton_(gamepad, int(button)) != 0; }

    bool GamepadApi::FirstTouch(Gamepad* gamepad, TouchFinger& finger, int& fingerCount) const
    {
        finger = {};
        fingerCount = 0;
        if (!gamepad || getNumTouchpads_(gamepad) <= 0) return false;
        const int count = getNumTouchpadFingers_(gamepad, 0);
        for (int index = 0; index < count; ++index)
        {
            uint8_t down = 0;
            float x = 0, y = 0, pressure = 0;
            if (!getTouchpadFinger_(gamepad, 0, index, &down, &x, &y, &pressure) || !down) continue;
            ++fingerCount;
            if (fingerCount == 1) finger = { true, x, y, pressure };
        }
        return fingerCount == 1;
    }
}
