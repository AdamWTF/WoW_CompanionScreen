// Discovery and loading of out-of-core extensions.
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

#include "runtime/Extensions.hpp"

#include "wxl/PluginApi.h"

#include "common/Log.hpp"
#include "engine/events/Event.hpp"
#include "engine/hook/Hook.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

namespace wxl::runtime::extensions
{
    namespace
    {
        // --- the service table an extension is handed ------------------------------------------
        // Every entry validates its own arguments: these are called across a binary boundary by code
        // this build never saw, so a null or an out-of-range id is an ordinary input, not a bug to
        // assert on.

        void __cdecl ApiLog(int level, const char* tag, const char* fmt, ...)
        {
            const int clamped = level < WXL_LOG_TRACE ? WXL_LOG_TRACE : level > WXL_LOG_ERROR ? WXL_LOG_ERROR : level;
            const auto severity = static_cast<log::Level>(clamped);
            if (!fmt || !log::Enabled(severity)) return;

            // Formatted here rather than passed through, so the tag cannot be lost and a format
            // string coming from another binary never reaches the sink unbounded.
            char line[1024];
            va_list args;
            va_start(args, fmt);
            std::vsnprintf(line, sizeof line, fmt, args);
            va_end(args);

            log::Write(severity, "%s: %s", tag ? tag : "extension", line);
        }

        void __cdecl ApiSubscribe(uint32_t event, WXL_EventFn handler, void* user)
        {
            if (!handler || event >= static_cast<uint32_t>(events::Event::Count))
            {
                WLOG_ERROR("extensions: subscribe to unknown event %u ignored", event);
                return;
            }
            events::Subscribe(static_cast<events::Event>(event), reinterpret_cast<events::Handler>(handler), user);
        }

        int __cdecl ApiHookAttach(const char* name, uintptr_t target, void* detour, void** original, int priority)
        {
            if (!target || !detour || !original) return 0;
            return hook::Install(name ? name : "extension", target, detour, original, priority) ? 1 : 0;
        }

        /// A service one extension offers another. The core keeps the name, the version and the
        /// pointer, and never looks at what they mean -- which is what lets two extensions agree on a
        /// capability this table knows nothing about.
        struct Service
        {
            std::string name;
            uint32_t    version;
            void*       iface;
        };

        std::vector<Service>& Services()
        {
            static std::vector<Service> services;
            return services;
        }

        void __cdecl ApiPublishInterface(const char* name, uint32_t version, void* iface)
        {
            if (!name || !iface) return;
            Services().push_back(Service{name, version, iface});
            WLOG_DEBUG("extensions: interface '%s' v%u published", name, version);
        }

        void* __cdecl ApiGetInterface(const char* name, uint32_t version)
        {
            if (!name) return nullptr;
            for (const Service& service : Services())
                if (service.version == version && service.name == name) return service.iface;
            return nullptr;
        }

        const WXL_Api g_api = {
            sizeof(WXL_Api),
            WXL_API_VERSION,
            &ApiLog,
            &ApiSubscribe,
            &ApiHookAttach,
            &ApiPublishInterface,
            &ApiGetInterface,
        };

        // --- loading ---------------------------------------------------------------------------

        /**
         * @brief Loads and initialises one extension.
         *
         * Everything that can disqualify it is checked before WXL_Load runs, which is the whole point
         * of the two-entry-point split: an extension built against another API version or another
         * client build is turned away having executed nothing.
         * @param folder  the extension's folder name, used for logging before its own name is known.
         * @param path    full path to its DLL.
         * @return true if the extension loaded and initialised.
         */
        bool LoadOne(const char* folder, const std::string& path)
        {
            HMODULE module = LoadLibraryA(path.c_str());
            if (!module)
            {
                WLOG_ERROR("extensions: '%s' could not be loaded (win32 %lu)", folder, GetLastError());
                return false;
            }

            auto query = reinterpret_cast<WXL_QueryFn>(GetProcAddress(module, "WXL_Query"));
            auto load  = reinterpret_cast<WXL_LoadFn>(GetProcAddress(module, "WXL_Load"));
            if (!query || !load)
            {
                WLOG_ERROR("extensions: '%s' exports no WXL_Query/WXL_Load", folder);
                FreeLibrary(module);
                return false;
            }

            const WXL_PluginInfo* info = query();
            if (!info || info->structSize != sizeof(WXL_PluginInfo))
            {
                WLOG_ERROR("extensions: '%s' returned no usable info block", folder);
                FreeLibrary(module);
                return false;
            }

            const char* name = info->name ? info->name : folder;
            if (info->apiVersion != WXL_API_VERSION)
            {
                WLOG_ERROR("extensions: '%s' was built against API v%u, this core serves v%u", name, info->apiVersion, WXL_API_VERSION);
                FreeLibrary(module);
                return false;
            }
            if (info->clientBuild != WXL_CLIENT_BUILD)
            {
                WLOG_ERROR("extensions: '%s' targets client build %u, this one is %u", name, info->clientBuild, WXL_CLIENT_BUILD);
                FreeLibrary(module);
                return false;
            }

            if (!load(&g_api))
            {
                // Deliberately still loaded: WXL_Load may already have attached a detour or taken a
                // subscription before deciding to give up, and unloading now would leave the core
                // pointing into freed code. A declining extension costs a module, never a crash.
                WLOG_ERROR("extensions: '%s' declined to initialise", name);
                return false;
            }

            WLOG_INFO("extensions: loaded '%s' v%u", name, info->pluginVersion);
            return true;
        }
    }

    int LoadAll()
    {
        std::vector<std::string> folders;

        WIN32_FIND_DATAA entry{};
        HANDLE search = FindFirstFileA("Extensions\\*", &entry);
        if (search == INVALID_HANDLE_VALUE)
        {
            WLOG_DEBUG("extensions: no Extensions folder");
            return 0;
        }
        do
        {
            if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (entry.cFileName[0] == '.') continue;
            folders.emplace_back(entry.cFileName);
        } while (FindNextFileA(search, &entry));
        FindClose(search);

        // Directory enumeration order is a filesystem detail; load order is something an extension
        // author can reason about, so it is sorted before anything is loaded.
        std::sort(folders.begin(), folders.end());

        int loaded = 0;
        for (const std::string& folder : folders)
        {
            const std::string path = "Extensions\\" + folder + "\\" + folder + ".dll";
            if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                WLOG_WARN("extensions: '%s' holds no %s.dll", folder.c_str(), folder.c_str());
                continue;
            }
            if (LoadOne(folder.c_str(), path)) ++loaded;
        }

        if (loaded || !folders.empty()) WLOG_INFO("extensions: %d loaded", loaded);
        return loaded;
    }
}
