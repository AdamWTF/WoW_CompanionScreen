// The extension ABI: what an out-of-core extension exports, and what the core hands it back.
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

#ifndef WXL_PLUGIN_API_H
#define WXL_PLUGIN_API_H

#include <stdint.h>

// The only file shared verbatim between the core and an extension, and deliberately the smallest
// one that can be: the SDK (wxl::game) is header-only and compiles into the extension, which reads
// the client directly because the image is fixed-base. Nothing that an extension can do on its own
// belongs here. What crosses is only what needs the core to arbitrate -- who detours what, who hears
// which event, where the log goes.
//
// Plain C with explicit __cdecl on every pointer: the default calling convention is a compiler flag,
// so an extension built with a different one would corrupt the stack on the first call if the
// convention were left implicit. Nothing with a C++ ABI (no std types, no exceptions, no vtables)
// crosses, so an extension is free to use a different toolchain or C++ standard library entirely.

#ifdef __cplusplus
extern "C" {
#endif

/// Bumped when the layout of anything below changes. An extension records the value it compiled
/// against, and the core refuses one it cannot serve.
#define WXL_API_VERSION 1

/// The client build an extension is written against. Refusing a mismatch here is what stops an
/// extension full of hardcoded addresses from running against an image where they mean nothing.
#define WXL_CLIENT_BUILD 12340

/// Severity values accepted by WXL_Api::Log, mirroring the core's own levels.
#define WXL_LOG_TRACE 0
#define WXL_LOG_DEBUG 1
#define WXL_LOG_INFO  2
#define WXL_LOG_WARN  3
#define WXL_LOG_ERROR 4

/// Chain position for HookAttach. Lower runs first, i.e. closest to the caller; equal values keep
/// attach order. Leave it at the default unless the extension must wrap, or be wrapped by, another.
#define WXL_HOOK_DEFAULT_PRIORITY 0

/**
 * @brief What an extension says about itself before any of its code runs.
 *
 * Returned by WXL_Query, which the core calls first and which must have no side effects: it is the
 * one moment where an incompatible extension can be turned away without having executed anything.
 */
typedef struct WXL_PluginInfo
{
    uint32_t    structSize;    ///< sizeof(WXL_PluginInfo), so the core can tell versions apart
    uint32_t    apiVersion;    ///< WXL_API_VERSION seen at compile time
    const char* name;          ///< short display name, used in log lines
    uint32_t    pluginVersion; ///< the extension's own version, opaque to the core
    uint32_t    clientBuild;   ///< WXL_CLIENT_BUILD seen at compile time
} WXL_PluginInfo;

/**
 * @brief Event handler signature.
 * @param user  the opaque pointer given to Subscribe.
 * @param args  the event's typed args struct, to be cast to the type documented for that event.
 */
typedef void(__cdecl* WXL_EventFn)(void* user, const void* args);

/**
 * @brief The core's services, handed to an extension at load.
 *
 * Read structSize before touching a field an older version may not have. The table lives for the
 * process lifetime, so an extension may keep the pointer.
 */
typedef struct WXL_Api
{
    uint32_t structSize;
    uint32_t apiVersion;

    /**
     * @brief Writes one line to the core's log.
     * @param level  one of the WXL_LOG_* values.
     * @param tag    short prefix identifying the caller, since one table serves every extension.
     * @param fmt    printf-style format, formatted by the core.
     */
    void(__cdecl* Log)(int level, const char* tag, const char* fmt, ...);

    /**
     * @brief Subscribes to a named core event for the process lifetime.
     *
     * There is deliberately no counterpart: removal would have to be handled inside the per-frame
     * publish path, and no extension is ever unloaded, so nothing would use it.
     * @param event    event id, from wxl::events::Event.
     * @param handler  invoked on every publication of that event.
     * @param user     opaque pointer passed back to the handler.
     */
    void(__cdecl* Subscribe)(uint32_t event, WXL_EventFn handler, void* user);

    /**
     * @brief Adds a detour to an address, alongside any other party already detouring it.
     *
     * original receives the next link in the chain, which ends at the engine function: calling
     * through it means "let the rest of the world handle this", and not calling it suppresses both
     * the parties behind and the engine function itself.
     * @param name      label used for logging.
     * @param target    address to detour.
     * @param detour    replacement function.
     * @param original  receives the next link in the chain.
     * @param priority  chain position; see WXL_HOOK_DEFAULT_PRIORITY.
     * @return non-zero if the detour was registered.
     */
    int(__cdecl* HookAttach)(const char* name, uintptr_t target, void* detour, void** original,
                             int priority);

    /**
     * @brief Offers a service to other extensions under a name the core never interprets.
     *
     * How two extensions agree on what iface points at is their business; the core only keeps the
     * name, version and pointer, so a capability can be added without touching this table.
     * @param name     agreed service name.
     * @param version  agreed service version.
     * @param iface    pointer to the service, valid for the process lifetime.
     */
    void(__cdecl* PublishInterface)(const char* name, uint32_t version, void* iface);

    /**
     * @brief Looks up a service published by another extension.
     * @param name     service name.
     * @param version  required version; a different published version does not match.
     * @return the published pointer, or NULL when no extension published it.
     */
    void*(__cdecl* GetInterface)(const char* name, uint32_t version);
} WXL_Api;

/// The two entry points as the core resolves them, by name, out of a loaded extension.
typedef const WXL_PluginInfo*(__cdecl* WXL_QueryFn)(void);
typedef int(__cdecl* WXL_LoadFn)(const WXL_Api* api);

// The prototypes themselves appear only for the extension being compiled -- the core includes this
// header for the table above and must not end up exporting the entry points it goes looking for.
#ifdef WXL_EXTENSION

/**
 * @brief First entry point: describes the extension. Must have no side effects.
 * @return the extension's info, or NULL to decline being loaded.
 */
__declspec(dllexport) const WXL_PluginInfo* __cdecl WXL_Query(void);

/**
 * @brief Second entry point: the extension sets itself up.
 *
 * Called on the main thread, after the graphics device exists and before the core's detour batch is
 * armed, so a detour attached here is enabled together with the core's own.
 * @param api  the core's service table, valid for the process lifetime.
 * @return non-zero on success; zero to abort this extension's load.
 */
__declspec(dllexport) int __cdecl WXL_Load(const WXL_Api* api);

#endif // WXL_EXTENSION

#ifdef __cplusplus
}
#endif

#endif // WXL_PLUGIN_API_H
