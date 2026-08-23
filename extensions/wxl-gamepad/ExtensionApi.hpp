// wxl-gamepad: SDL3 controller input for WarcraftXL.
// Copyright (C) 2026 WarcraftXL contributors. GPL-3.0-or-later.
#pragma once

#include "wxl/PluginApi.h"

#include <cstdint>

namespace wxl_gamepad
{
    inline constexpr const char* kTag = "wxl-gamepad";
    extern const WXL_Api* g_api;

    inline void Log(int level, const char* message)
    {
        if (g_api && g_api->Log) g_api->Log(level, kTag, "%s", message);
    }

    bool InstallGamepad();
}
