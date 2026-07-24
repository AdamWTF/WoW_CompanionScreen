// Native split-ADT reader: split-map detection (the once-per-map _tex0 probe + MPHD alpha fix).
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

#include "client/CMapArea/AdtSplitInternal.hpp"

#include "common/Log.hpp"

#include <cstring>
#include <string>

namespace
{
    using namespace wxl::runtime::adtsplit::detail;

    /**
     * @brief Derives the per-map cache key from a tile filename.
     *
     * "<dir>\<name>_%d_%d.adt" -> "<dir>\<name>". Returns false when the name does not look like a tile
     * path (no ".adt" tail or no two trailing _<digits> groups) -- such a name is never split.
     */
    bool MapKeyFromTileName(const char* name, std::string& key)
    {
        const size_t len = name ? std::strlen(name) : 0;
        if (len < 9) return false; // "a_0_0.adt" is the shortest plausible
        const char* ext = name + len - 4;
        if (_stricmp(ext, ".adt") != 0) return false;

        // scan back across two _<digits> groups
        const char* p = ext;
        for (int group = 0; group < 2; ++group)
        {
            const char* d = p;
            while (d > name && d[-1] >= '0' && d[-1] <= '9') --d;
            if (d == p || d == name || d[-1] != '_') return false;
            p = d - 1;
        }
        key.assign(name, static_cast<size_t>(p - name));
        return true;
    }

    /**
     * @brief One-time MPHD compatibility fix for split maps: Legion-side `adt_has_height_texturing`
     *        (0x80) implies 4096-byte alpha maps, which 3.3.5's unpack sites size from bit2 only -- so
     *        bit2 is OR'd in when 0x80 is set without it. The flags dword is the live WDT MPHD copy the
     *        alpha unpackers consult at every build.
     */
    void ApplyMphdAlphaFix()
    {
        uint32_t& flags = *reinterpret_cast<uint32_t*>(adt::kMphdFlags);
        if ((flags & 0x80u) != 0 && (flags & 0x4u) == 0)
        {
            flags |= 0x4u;
            WLOG_INFO("adt-split: MPHD height-texturing flag present, forcing big-alpha (bit2) for unpack sizing");
        }
    }
}

namespace wxl::runtime::adtsplit::detail
{
    /**
     * @brief Reports (and lazily probes) whether the map a tile filename belongs to is split.
     *
     * The probe opens "<prefix>_%d_%d_tex0.adt" of THIS tile once through the storage seam and caches
     * the answer per map prefix -- an all-or-nothing-per-map dataset assumption, matching how repacks
     * ship. Cost: one open/close per map per session.
     */
    bool IsSplitTileName(const char* name, std::string& keyOut)
    {
        if (!MapKeyFromTileName(name, keyOut)) return false;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_splitMaps.find(keyOut);
            if (it != g_splitMaps.end()) return it->second;
        }
        const size_t len = std::strlen(name);
        std::string probe(name, len - 4);
        probe += "_tex0.adt";
        void* h = OpenFile(probe.c_str());
        const bool split = h != nullptr;
        if (h) CloseFile(h);
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_splitMaps[keyOut] = split;
        }
        if (split)
        {
            g_statSplitMaps.fetch_add(1, std::memory_order_relaxed);
            ApplyMphdAlphaFix();
            WLOG_INFO("adt-split: map '%s' detected as SPLIT (Cata+ root/_tex0/_obj0)", keyOut.c_str());
        }
        return split;
    }
}
