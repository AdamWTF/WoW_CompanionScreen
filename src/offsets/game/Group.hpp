// Party-operation landmarks for WoW 3.3.5a build 12340.
#pragma once

#include <cstdint>

namespace wxl::offsets::game::group
{
    // CGPlayer::Uninvite at the endpoint used by the stock UninviteUnit FrameScript wrapper.
    // The wrapper resolves the supplied name to a GUID, applies its protected-call gate, then
    // invokes this method with the GUID and an optional Dungeon Finder reason.
    constexpr uintptr_t kUninvite = 0x006D43C0;
    using UninviteFn = void(__thiscall*)(void* player, unsigned long long guid, const char* reason);
}
