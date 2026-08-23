// Typed native action-bar dispatch for WoW 3.3.5a build 12340.
#pragma once

#include "game/Binding.hpp"
#include "offsets/engine/Action.hpp"

namespace wxl::game::action
{
    namespace off = wxl::offsets::engine::action;

    /// Executes one normal action-bar slot. Slots are one-based to match the WoW UI (1..120).
    inline bool Use(int slot)
    {
        if (slot < 1 || slot > 120) return false;
        const off::TargetContext noTarget{};
        Native<off::UseActionFn>(off::kUseAction)(slot - 1, &noTarget, nullptr);
        return true;
    }
}
