// Typed native party operations for WoW 3.3.5a build 12340.
#pragma once

#include "game/Binding.hpp"
#include "game/World.hpp"
#include "offsets/game/Group.hpp"

namespace wxl::game::group
{
    namespace off = wxl::offsets::game::group;

    /// Removes the authoritative party GUID through the same client method used by UninviteUnit.
    /// Callers must enforce current roster, leadership, group-type and session eligibility first.
    inline bool Remove(unsigned long long guid)
    {
        if (!guid) return false;
        void* const player = world::ResolveObject(world::ActivePlayerGuid(), world::kTypeMaskPlayer);
        if (!player) return false;
        Native<off::UninviteFn>(off::kUninvite)(player, guid, "");
        return true;
    }
}
