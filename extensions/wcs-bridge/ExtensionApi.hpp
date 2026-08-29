#pragma once

#include "wxl/PluginApi.h"

namespace wcs_bridge
{
    inline constexpr const char* kTag = "wcs-bridge";
    extern const WXL_Api* g_api;
    inline void Log(int level, const char* message) { if (g_api && g_api->Log) g_api->Log(level, kTag, "%s", message); }
}
