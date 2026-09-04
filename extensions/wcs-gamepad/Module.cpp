#include "ExtensionApi.hpp"
#include "game/Script.hpp"

namespace wcs_gamepad
{
    namespace
    {
        wxl::game::script::ValidateCallbackFn g_validateOriginal = nullptr;
        void __cdecl ValidateDetour(uintptr_t function) { if (!IsOwnLuaFunction(function)) g_validateOriginal(function); }
    }
}

const WXL_PluginInfo* __cdecl WXL_Query(void)
{
    static const WXL_PluginInfo info = {
        sizeof(WXL_PluginInfo), WXL_API_VERSION, "wcs-gamepad", 0x010002, WXL_CLIENT_BUILD,
    };
    return &info;
}

int __cdecl WXL_Load(const WXL_Api* api)
{
    if (!api || api->apiVersion != WXL_API_VERSION || api->structSize < sizeof(WXL_Api)) return 0;
    wcs_gamepad::g_api = api;
    wcs_gamepad::Log(WXL_LOG_INFO, "initialising WoW Companion Screen gamepad 1.0.2 for WoW 3.3.5a build 12340");
    if (!api->HookAttachByName("Lua.ValidateFunctionPointer", reinterpret_cast<void*>(&wcs_gamepad::ValidateDetour), reinterpret_cast<void**>(&wcs_gamepad::g_validateOriginal), WXL_HOOK_DEFAULT_PRIORITY)) return 0;
    return wcs_gamepad::InstallGamepad() ? 1 : 0;
}
