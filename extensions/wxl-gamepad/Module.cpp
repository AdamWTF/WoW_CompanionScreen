#include "ExtensionApi.hpp"

const WXL_PluginInfo* __cdecl WXL_Query(void)
{
    static const WXL_PluginInfo info = {
        sizeof(WXL_PluginInfo), WXL_API_VERSION, "wxl-gamepad", 2, WXL_CLIENT_BUILD,
    };
    return &info;
}

int __cdecl WXL_Load(const WXL_Api* api)
{
    if (!api || api->apiVersion != WXL_API_VERSION || api->structSize < sizeof(WXL_Api)) return 0;
    wxl_gamepad::g_api = api;
    wxl_gamepad::Log(WXL_LOG_INFO, "initialising for WoW 3.3.5a build 12340");
    return wxl_gamepad::InstallGamepad() ? 1 : 0;
}
