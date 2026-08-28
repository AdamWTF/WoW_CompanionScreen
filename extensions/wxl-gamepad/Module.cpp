#include "ExtensionApi.hpp"
#include "game/Script.hpp"

namespace wxl_gamepad
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
        sizeof(WXL_PluginInfo), WXL_API_VERSION, "wxl-gamepad", 4, WXL_CLIENT_BUILD,
    };
    return &info;
}

int __cdecl WXL_Load(const WXL_Api* api)
{
    if (!api || api->apiVersion != WXL_API_VERSION || api->structSize < sizeof(WXL_Api)) return 0;
    wxl_gamepad::g_api = api;
    wxl_gamepad::Log(WXL_LOG_INFO, "initialising for WoW 3.3.5a build 12340");
    if (!api->HookAttachByName("Lua.ValidateFunctionPointer", reinterpret_cast<void*>(&wxl_gamepad::ValidateDetour), reinterpret_cast<void**>(&wxl_gamepad::g_validateOriginal), WXL_HOOK_DEFAULT_PRIORITY)) return 0;
    return wxl_gamepad::InstallGamepad() ? 1 : 0;
}
