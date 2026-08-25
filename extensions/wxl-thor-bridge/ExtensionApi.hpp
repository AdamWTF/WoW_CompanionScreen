#pragma once

#include "wxl/PluginApi.h"

namespace wxl_thor
{
    inline constexpr const char* kTag = "wxl-thor-bridge";
    extern const WXL_Api* g_api;
    inline void Log(int level, const char* message) { if (g_api && g_api->Log) g_api->Log(level, kTag, "%s", message); }
}
