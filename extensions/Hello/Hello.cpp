// Smoke-test extension: proves the core still discovers, validates and serves an out-of-core DLL.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

// The smallest extension that exercises every part of the boundary: it is queried, it is validated,
// it is handed the table, it takes a subscription, and the subscription fires. Kept permanently
// rather than thrown away after the first run -- "does the core still load an extension" is a
// question worth being able to answer after any change to the loader or the ABI.

#define WXL_EXTENSION
#include "wxl/PluginApi.h"

#include "engine/events/Event.hpp"

namespace
{
    const WXL_Api* g_api = nullptr;
    bool           g_announced = false;

    const WXL_PluginInfo g_info = {
        sizeof(WXL_PluginInfo),
        WXL_API_VERSION,
        "Hello",
        1,
        WXL_CLIENT_BUILD,
    };

    /// Announces only the first frame: the point is that a handler registered across the ABI reaches
    /// the core's publish path at all, and a line per frame would drown the log proving it.
    void __cdecl OnFrame(void* user, const void*)
    {
        bool* announced = static_cast<bool*>(user);
        if (!announced || *announced) return;
        *announced = true;
        g_api->Log(WXL_LOG_INFO, "Hello", "first frame reached the extension");
    }
}

const WXL_PluginInfo* __cdecl WXL_Query(void)
{
    return &g_info;
}

int __cdecl WXL_Load(const WXL_Api* api)
{
    if (!api || api->apiVersion != WXL_API_VERSION) return 0;
    g_api = api;

    api->Log(WXL_LOG_INFO, "Hello", "loaded from Extensions/, subscribing to OnFrame");
    api->Subscribe(static_cast<uint32_t>(wxl::events::Event::OnFrame), &OnFrame, &g_announced);
    return 1;
}
