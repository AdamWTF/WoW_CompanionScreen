// Native selection and interaction bindings for WoW 3.3.5a build 12340.
// Copyright (C) 2026 WarcraftXL contributors. GPL-3.0-or-later.
#pragma once

#include "game/Binding.hpp"
#include "offsets/game/Unit.hpp"

#include <cstddef>
#include <cstdint>

namespace wxl::game::interaction
{
    namespace off = wxl::offsets::game::unit;

    template <class Fn>
    inline Fn Virtual(void* object, size_t slot)
    { return Fn((*static_cast<void***>(object))[slot]); }

    inline uint32_t Field(void* object, size_t index)
    {
        if (!object) return 0;
        const auto* base = static_cast<const off::ObjectBase*>(object);
        return base->header ? reinterpret_cast<const uint32_t*>(base->header)[index] : 0;
    }

    inline uint32_t Entry(void* object) { return Field(object, off::kObjectEntryField); }
    inline uint32_t UnitFlags(void* object) { return Field(object, off::kUnitFlagsField); }
    inline uint32_t UnitDynamicFlags(void* object) { return Field(object, off::kUnitDynamicFlagsField); }
    inline uint32_t UnitNpcFlags(void* object) { return Field(object, off::kUnitNpcFlagsField); }
    inline uint32_t GameObjectDisplayId(void* object) { return Field(object, off::kGameObjectDisplayField); }
    inline uint32_t GameObjectFlags(void* object) { return Field(object, off::kGameObjectFlagsField); }
    inline uint32_t GameObjectDynamic(void* object) { return Field(object, off::kGameObjectDynamicField); }

    inline bool GameObjectInteractable(void* object)
    {
        return GameObjectDisplayId(object) != 0 &&
            (GameObjectFlags(object) & off::kGameObjectFlagNotSelectable) == 0;
    }

    inline bool InCombat(void* player)
    { return (UnitFlags(player) & off::kUnitFlagInCombat) != 0; }

    inline bool Selectable(void* object)
    {
        return object && Virtual<off::SelectableFn>(object, off::kVtSelectable)(object) != 0;
    }

    inline float Facing(void* object)
    { return object ? Virtual<off::FacingFn>(object, off::kVtFacing)(object) : 0.0f; }

    inline const char* Name(void* object)
    {
        if (!object) return "";
        const char* value = Virtual<off::NameFn>(object, off::kVtName)(object);
        return value ? value : "";
    }

    inline void Target(unsigned long long guid)
    {
        Native<off::TargetGuidFn>(off::kTargetGuidSet)(uint32_t(guid), uint32_t(guid >> 32));
    }

    inline bool Interact(unsigned long long guid)
    {
        return Native<off::InteractGuidFn>(off::kInteractGuid)(uint32_t(guid), uint32_t(guid >> 32)) != 0;
    }
}
