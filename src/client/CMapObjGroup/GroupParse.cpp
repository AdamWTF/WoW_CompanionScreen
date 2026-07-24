// CMapObjGroup parse detour: publish OnWmoGroupLoad before the native sub-chunk walk.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "engine/hook/Hook.hpp"
#include "engine/hook/Registry.hpp"
#include "engine/events/Event.hpp"
#include "engine/diag/AssetProfile.hpp"

#include "offsets/game/WMO.hpp"

#include <cstdint>

namespace
{
    namespace ev    = wxl::events;
    namespace wmo   = wxl::offsets::game::wmo;
    namespace aprof = wxl::runtime::assetprof;

    wmo::WmoGroup_ParseFn g_origWmoGroup = nullptr;

    /**
     * @brief Detours WMO group parse, emitting OnWmoGroupLoad before the native sub-chunk walk.
     *
     * The join point of the sync and async group-load paths, before the sub-chunk walk, so a subscriber
     * may reshape the group buffer in place; the native walk then reads the reshaped bytes.
     * @param group  map-object group whose buffer was just read.
     * @param edx    unused register slot for the thiscall convention.
     */
    void __fastcall hkWmoGroupParse(void* group, void* edx)
    {
        const uint64_t preStarted = aprof::Now();
        ev::WmoGroupLoadArgs a{ group };
        ev::Emit(ev::Event::OnWmoGroupLoad, &a);
        if (preStarted) aprof::Record(aprof::Phase::WmoGroupPre, aprof::Now() - preStarted);
        const uint64_t nativeStarted = aprof::Now();
        g_origWmoGroup(group, edx);
        if (nativeStarted) aprof::Record(aprof::Phase::WmoGroupNative, aprof::Now() - nativeStarted);
    }

    bool InstallWmoGroupParse()
    {
        wxl::hook::Install("WmoGroupParse", wmo::kGroupParse, &hkWmoGroupParse, &g_origWmoGroup);
        return true;
    }
}

WXL_REGISTER_FEATURE("wmo-group-parse", true, InstallWmoGroupParse)
