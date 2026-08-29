#include "XInputBackend.hpp"
#include "ExtensionApi.hpp"
#include "Processing.hpp"

namespace wcs_gamepad
{
    namespace
    {
        constexpr unsigned long kSuccess = 0;
        constexpr unsigned short kDpadUp = 0x0001, kDpadDown = 0x0002, kDpadLeft = 0x0004, kDpadRight = 0x0008;
        constexpr unsigned short kStart = 0x0010, kBack = 0x0020, kLeftThumb = 0x0040, kRightThumb = 0x0080;
        constexpr unsigned short kLeftShoulder = 0x0100, kRightShoulder = 0x0200;
        constexpr unsigned short kSouth = 0x1000, kEast = 0x2000, kWest = 0x4000, kNorth = 0x8000;
    }
    bool XInputControllerBackend::Initialize()
    {
        static constexpr const char* libraries[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
        for (const char* library : libraries)
        {
            module_ = LoadLibraryA(library); if (!module_) continue;
            getState_ = reinterpret_cast<GetStateFn>(GetProcAddress(module_, "XInputGetState"));
            if (getState_) { g_api->Log(WXL_LOG_INFO, kTag, "[XInput] Library loaded: %s", library); break; }
            FreeLibrary(module_); module_ = nullptr;
        }
        if (!getState_) { Log(WXL_LOG_WARN, "[XInput] no compatible library available"); return false; }
        for (unsigned index = 0; index < 4; ++index)
        {
            State state{}; const bool available = getState_(index, &state) == kSuccess;
            g_api->Log(WXL_LOG_INFO, kTag, "[XInput] Slot %u: %s", index, available ? "connected" : "unavailable");
            if (available && !connected_) { connected_ = true; slot_ = index; }
        }
        if (connected_) { info_.backend = BackendKind::XInput; info_.index = int(slot_); info_.name = "XInput Controller (Slot " + std::to_string(slot_) + ")"; info_.mapped = true; info_.glyphHint = "Xbox"; }
        return true;
    }
    void XInputControllerBackend::Shutdown() { connected_ = false; getState_ = nullptr; if (module_) FreeLibrary(module_); module_ = nullptr; }
    bool XInputControllerBackend::Poll(ControllerState& output)
    {
        if (!connected_ || !getState_) return false; State value{};
        if (getState_(slot_, &value) != kSuccess) { connected_ = false; return false; }
        const Gamepad& p = value.gamepad; ControllerState s{};
        s.south = (p.buttons & kSouth) != 0; s.east = (p.buttons & kEast) != 0; s.west = (p.buttons & kWest) != 0; s.north = (p.buttons & kNorth) != 0;
        s.dpadUp = (p.buttons & kDpadUp) != 0; s.dpadDown = (p.buttons & kDpadDown) != 0; s.dpadLeft = (p.buttons & kDpadLeft) != 0; s.dpadRight = (p.buttons & kDpadRight) != 0;
        s.leftShoulder = (p.buttons & kLeftShoulder) != 0; s.rightShoulder = (p.buttons & kRightShoulder) != 0; s.leftStickButton = (p.buttons & kLeftThumb) != 0; s.rightStickButton = (p.buttons & kRightThumb) != 0;
        s.start = (p.buttons & kStart) != 0; s.back = (p.buttons & kBack) != 0; s.leftTrigger = float(p.leftTrigger) / 255.0f; s.rightTrigger = float(p.rightTrigger) / 255.0f;
        s.leftX = NormalizeSigned16(p.leftX); s.leftY = -NormalizeSigned16(p.leftY); s.rightX = NormalizeSigned16(p.rightX); s.rightY = -NormalizeSigned16(p.rightY);
        output = s; return true;
    }
}
