// Native action-bar dispatch for WoW 3.3.5a build 12340.
#pragma once

#include <cstdint>

namespace wxl::offsets::engine::action
{
    // The non-FrameScript action executor reached by UseAction at 0x005AC05F. It validates the
    // zero-based action slot (0..143), resolves the action, and drives the spell/item/macro path.
    // __cdecl(slotIndex, optionalTargetGuid, optionalTargetName). The stock Lua wrapper passes an
    // eight-byte zero target context and null name for UseAction(slot, 0).
    constexpr uintptr_t kUseAction = 0x005ABBC0;

    struct TargetContext { uint32_t low; uint32_t high; };
    using UseActionFn = void(__cdecl*)(int slotIndex, const TargetContext* target, const char* targetName);
}
