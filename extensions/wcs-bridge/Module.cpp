#include "ExtensionApi.hpp"
#include "WcsBridge.hpp"

#include "engine/events/Event.hpp"
#include "game/Script.hpp"

namespace wcs_bridge
{
    const WXL_Api* g_api = nullptr;
    namespace
    {
        using ValidateFn = wxl::game::script::ValidateCallbackFn;
        using GlueRenderFn = void(__cdecl*)(void*);
        ValidateFn g_validateOriginal = nullptr;
        GlueRenderFn g_glueRenderOriginal = nullptr;
        void* g_loadingEnableOriginal = nullptr;

        template <class Fn> void Isolate(Fn&& fn) { try { fn(); } catch (...) { Log(WXL_LOG_ERROR, "isolated an internal bridge failure"); } }
        void __cdecl ValidateDetour(uintptr_t function) { if (!g_bridge || !g_bridge->IsOwnLuaFunction(function)) g_validateOriginal(function); }
        void __cdecl GlueRenderDetour(void* frame) { g_glueRenderOriginal(frame); Isolate([] { if (g_bridge) g_bridge->OnGlueRender(); }); }
        void __cdecl LoadingNotice() { Isolate([] { if (g_bridge) g_bridge->OnLoading(); }); }
        // This verified hook point is __cdecl but its map-key argument is intentionally not part of
        // the extension ABI. Preserve the complete incoming stack/register contract and tail-jump to
        // the hook trampoline after observing the edge.
        __declspec(naked) void LoadingEnableDetour()
        {
            __asm
            {
                pushfd
                pushad
                call LoadingNotice
                popad
                popfd
                jmp dword ptr [g_loadingEnableOriginal]
            }
        }
        void __cdecl Update(void*, const void* args) { Isolate([&] { if (g_bridge) g_bridge->OnUpdate(*static_cast<const wxl::events::UpdateArgs*>(args)); }); }
        void __cdecl WorldEnter(void*, const void* args) { Isolate([&] { if (g_bridge) g_bridge->OnWorldEnter(*static_cast<const wxl::events::WorldEnterArgs*>(args)); }); }
        void __cdecl WorldLeave(void*, const void* args) { Isolate([&] { if (g_bridge) g_bridge->OnWorldLeave(*static_cast<const wxl::events::WorldLeaveArgs*>(args)); }); }
        void __cdecl Panel(void*) { Isolate([] { if (g_bridge) g_bridge->DrawPanel(); }); }
    }
}

const WXL_PluginInfo* __cdecl WXL_Query(void)
{
    static const WXL_PluginInfo info = {sizeof(WXL_PluginInfo), WXL_API_VERSION, "wcs-bridge", 0x010002, WXL_CLIENT_BUILD}; return &info;
}

int __cdecl WXL_Load(const WXL_Api* api)
{
    using namespace wcs_bridge;
    if (!api || api->apiVersion != WXL_API_VERSION || api->structSize < sizeof(WXL_Api)) return 0; g_api = api;
    try
    {
        static WcsBridge bridge; g_bridge = &bridge;
        if (!api->HookAttachByName("Lua.ValidateFunctionPointer", reinterpret_cast<void*>(&ValidateDetour), reinterpret_cast<void**>(&g_validateOriginal), WXL_HOOK_DEFAULT_PRIORITY)) return 0;
        if (!api->HookAttachByName("Gx.GlueModelRender", reinterpret_cast<void*>(&GlueRenderDetour), reinterpret_cast<void**>(&g_glueRenderOriginal), WXL_HOOK_DEFAULT_PRIORITY)) return 0;
        if (!api->HookAttachByName("World.LoadingScreenEnable", reinterpret_cast<void*>(&LoadingEnableDetour), &g_loadingEnableOriginal, WXL_HOOK_DEFAULT_PRIORITY)) return 0;
        api->Subscribe(uint32_t(wxl::events::Event::OnUpdate), &Update, nullptr); api->Subscribe(uint32_t(wxl::events::Event::OnWorldEnter), &WorldEnter, nullptr);
        api->Subscribe(uint32_t(wxl::events::Event::OnWorldLeave), &WorldLeave, nullptr); api->UiAddPanel("wcs-bridge", &Panel, nullptr);
        Log(WXL_LOG_INFO, "initialising WoW Companion Screen bridge 1.0.2 for WoW 3.3.5a build 12340"); return bridge.Initialise() ? 1 : 0;
    }
    catch (...) { Log(WXL_LOG_ERROR, "initialisation failed with an internal exception"); return 0; }
}
