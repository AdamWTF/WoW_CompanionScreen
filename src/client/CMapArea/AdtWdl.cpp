// Native split-ADT reader: the Cata+ .wdl direct reader (distant low-detail terrain).
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

// A Cata+ .wdl puts MLDD/MLDX/MLDF/MLMD/MLMX (low-detail doodad/wmo placements) between MVER and the
// chunks 3.3.5 knows, and inserts MAOE between each tile's MARE and MAHO. The native CMap::LoadWdl only
// tests for MWMO after MVER; on a miss it treats whatever sits there as the MAOF offset table, so an
// MLDD chunk's bytes become "file offsets", the 64x64 grid loop derefs garbage and the client #132s.
//
// Same philosophy as the split-ADT direct-fill: READ the real format and fill the stock WDL runtime
// (kWdlState) with the true data -- a byte-faithful replay of the native grid loop with the ML* family
// skipped and MAOE stepped over. Distant terrain renders from the REAL MARE heights.
//
// TODO(adt-split/wdl-wmo): distant WMOs. A Cata+ WDL carries them as MLDD/MLDX (doodads) and MLMD/MLMX
// (wmos) instead of 3.3.5's MWMO/MWID/MODF. wdlState[1..4] stay 0 for now.

#include "engine/hook/Hook.hpp"
#include "client/CMapArea/AdtSplitInternal.hpp"

#include "common/Log.hpp"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
    using namespace wxl::runtime::adtsplit::detail;

    adt::LoadWdlFn g_origLoadWdl = nullptr;

    enum class WdlKind { Missing, Stock, CataPlus };

    /// Bounds-checked sniff of the first bytes of "<mapPath>\<mapName>.wdl": reads MVER and the tag of
    /// the chunk after it into a stack buffer. "ML.." there = Cata+ family. Malformed or absent input
    /// never faults (fixed-size local reads only) and classifies as Stock/Missing so the native path
    /// keeps its behaviour. No unwindable locals (SEH shell wraps it, C2712).
    WdlKind SniffWdlKind(const char* mapPath, const char* mapName)
    {
        char path[0x120];
        std::snprintf(path, sizeof path, "%s\\%s.wdl", mapPath, mapName);
        void* h = OpenFile(path);
        if (!h) return WdlKind::Missing;
        uint8_t head[0x20] = { 0 };
        const bool ok = ReadBytes(h, head, sizeof head);
        CloseFile(h);
        if (!ok || Rd32(head) != FourCC("MVER")) return WdlKind::Stock;
        const uint32_t mverSize = Rd32(head + 4);
        if (mverSize > sizeof head - 12) return WdlKind::Stock; // tag after MVER not in the window
        const uint32_t tag = Rd32(head + 8 + mverSize);
        return (tag >> 16) == ((uint32_t('M') << 8) | uint32_t('L')) ? WdlKind::CataPlus
                                                                     : WdlKind::Stock;
    }

    /// SEH shell: any fault while touching the real WDL bytes/SFile internals classifies as Stock
    /// (native behaviour unchanged), never a crash.
    WdlKind SniffWdlKindGuarded(const char* mapPath, const char* mapName)
    {
        __try
        {
            return SniffWdlKind(mapPath, mapName);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return WdlKind::Stock;
        }
    }

    /// One CMapAreaLow slot fill, byte-faithful to the native grid-loop body (decompile lines
    /// ~808711-808781): obj+0x44 = MARE data, 545-short min/max scan, the exact bounds/center/radius
    /// float math (same literals the native constants round to), and the MAHO budget count (0xC per
    /// un-holed cell). `mahoData` may be null (no holes -> full 0xC00 budget, like stock).
    void FillAreaLow(void* obj, uint32_t col, uint32_t row, uint8_t* mareData, const uint8_t* mahoData)
    {
        At<uint8_t*>(obj, 0x44) = mareData;

        // native: psVar9 = (short*)(obj+0x44) + 2, reads [-2..+2] over 0x6D iterations = 545 s16
        const int16_t* hp = reinterpret_cast<const int16_t*>(mareData);
        int32_t mx = -100000, mn = 100000; // native seeds 0xFFFE7960 / 100000
        for (uint32_t i = 0; i < 545; ++i)
        {
            const int32_t v = hp[i];
            if (v > mx) mx = v;
            if (v < mn) mn = v;
        }

        At<int32_t>(obj, 0x38) = static_cast<int32_t>(col);
        At<int32_t>(obj, 0x3C) = static_cast<int32_t>(row);
        // native literals: 33.333332 (= tileSize/16), 17066.666 (grid origin), 533.3333 (tile size)
        const float x = -(static_cast<float>(row * 16u) * 33.333332f) + 17066.666f;
        const float y = -(static_cast<float>(col * 16u) * 33.333332f) + 17066.666f;
        At<float>(obj, 0x2C) = x;
        At<float>(obj, 0x10) = x;
        At<float>(obj, 0x30) = y;
        At<float>(obj, 0x14) = y;
        At<float>(obj, 0x18) = static_cast<float>(mx);
        At<float>(obj, 0x04) = x - 533.3333f;
        At<float>(obj, 0x08) = y - 533.3333f;
        At<float>(obj, 0x0C) = static_cast<float>(mn);
        At<float>(obj, 0x1C) = (At<float>(obj, 0x10) + At<float>(obj, 0x04)) * 0.5f;
        At<float>(obj, 0x20) = (At<float>(obj, 0x08) + At<float>(obj, 0x14)) * 0.5f;
        At<float>(obj, 0x24) = (At<float>(obj, 0x0C) + At<float>(obj, 0x18)) * 0.5f;
        const float dx = At<float>(obj, 0x10) - At<float>(obj, 0x1C);
        const float dy = At<float>(obj, 0x14) - At<float>(obj, 0x20);
        const float dz = At<float>(obj, 0x18) - At<float>(obj, 0x24);
        At<float>(obj, 0x28) = std::sqrt(dz * dz + dy * dy + dx * dx);

        if (mahoData)
        {
            At<const uint8_t*>(obj, 0x48) = mahoData;
            uint32_t budget = 0;
            const uint16_t* masks = reinterpret_cast<const uint16_t*>(mahoData);
            for (uint32_t m = 0; m < 16; ++m)
                for (uint32_t bit = 0; bit < 16; ++bit)
                    if ((masks[m] & (1u << bit)) == 0)
                        budget += 0xC;
            At<uint32_t>(obj, 0x40) = budget;
        }
        else
        {
            At<uint32_t>(obj, 0x48) = 0;
            At<uint32_t>(obj, 0x40) = 0xC00; // 16x16 cells x 0xC, no holes
        }
    }

    /**
     * @brief Reads a Cata+ WDL directly into the stock WDL runtime (wdlState), replaying the native
     *        CMap::LoadWdl body over the real layout.
     *
     * Prologue like native: open through the storage seam, size, CMap::AllocRawAreaData (so the native
     * unload's SMemFree pairs), whole-file read, buffer -> wdlState[0]. Then a bounds-checked top-level
     * walk skips the ML* placement family and points wdlState[5] at the REAL MAOF data; wdlState[1..4]
     * (MWMO/MWID/MODF) stay 0. The grid loop then replays the native per-tile body for every non-zero
     * MAOF offset, skipping the Cata+ MAOE chunk sitting between MARE and MAHO.
     *
     * @return 1 like the native success path; 0 with wdlState[0]/[5] = 0 (the native missing-file
     *         end-state) when the file cannot be fetched or has no usable MAOF.
     */
    uint32_t ReadCataWdl(int* wdlState, const char* mapPath, const char* mapName)
    {
        char path[0x120];
        std::snprintf(path, sizeof path, "%s\\%s.wdl", mapPath, mapName);
        void* h = OpenFile(path);
        if (!h) return 0;
        const uint32_t size = SizeOfFile(h);
        if (size == 0 || size == 0xFFFFFFFFu) { CloseFile(h); return 0; }
        uint8_t* buf = AllocRaw(size);
        if (!buf) { CloseFile(h); return 0; }
        const bool ok = ReadBytes(h, buf, size);
        CloseFile(h);
        if (!ok) { FreeRaw(buf, size); return 0; }

        // --- top-level walk: skip MVER + ML*, find the real MAOF ---
        uint8_t* maofData = nullptr;
        uint32_t maofSize = 0, mlChunks = 0;
        WalkTop(buf, size, [&](uint32_t tag, uint8_t* hdr, uint32_t sz) {
            if (tag == FourCC("MAOF")) { maofData = hdr + 8; maofSize = sz; }
            else if ((tag >> 16) == ((uint32_t('M') << 8) | uint32_t('L'))) ++mlChunks;
        });
        if (!maofData || maofSize < adt::kWdlSlotCount * 4)
        {
            FreeRaw(buf, size);
            WLOG_ERROR("adt-split: Cata+ WDL '%s' has no usable MAOF (size %u); no distant terrain",
                       path, maofSize);
            return 0;
        }
        wdlState[0] = reinterpret_cast<int>(buf);
        wdlState[5] = reinterpret_cast<int>(maofData);
        // wdlState[1..4] = MWMO/MWID/MODF: left at their clean zeroes (distant-WMO TODO above)

        // --- the native grid loop over the 64x64 MAOF offsets, bounds-checked ---
        uint32_t tiles = 0, holed = 0, badSlots = 0;
        for (uint32_t row = 0; row < 64; ++row)
        {
            for (uint32_t col = 0; col < 64; ++col)
            {
                const uint32_t idx = row * 64u + col;
                const uint32_t off = Rd32(maofData + idx * 4);
                if (off == 0) continue;
                // MARE chunk at the absolute file offset (native adds it to the buffer base blind;
                // we verify tag + that the 545-short heightmap is inside the file)
                if (off + 8 > size || size - (off + 8) < 545u * 2u ||
                    Rd32(buf + off) != FourCC("MARE"))
                {
                    ++badSlots;
                    continue;
                }
                uint8_t* mareHdr  = buf + off;
                uint8_t* mareData = mareHdr + 8;
                const uint32_t mareSize = Rd32(mareHdr + 4);

                // chunk after MARE: 3.3.5 puts MAHO right there; Cata+ inserts MAOE first. Walk a few
                // headers, skipping foreigners, until MAHO / the next tile / the buffer end.
                const uint8_t* mahoData = nullptr;
                if (mareSize <= size - (off + 8))
                {
                    uint32_t nextOff = off + 8 + mareSize;
                    for (int hop = 0; hop < 3 && nextOff + 8 <= size; ++hop)
                    {
                        const uint32_t tag = Rd32(buf + nextOff);
                        const uint32_t sz  = Rd32(buf + nextOff + 4);
                        if (sz > size - nextOff - 8) break;
                        if (tag == FourCC("MAHO"))
                        {
                            if (sz >= 16u * 2u) mahoData = buf + nextOff + 8;
                            break;
                        }
                        if (tag == FourCC("MARE")) break; // next tile, no MAHO for this one
                        nextOff += 8 + sz;                // skip MAOE (or other foreigners)
                    }
                }

                void* obj = wxl::game::Native<adt::AllocAreaLowFn>(adt::kAllocAreaLow)();
                if (!obj) { ++badSlots; continue; }
                wdlState[6 + idx] = reinterpret_cast<int>(obj);
                FillAreaLow(obj, col, row, mareData, mahoData);
                ++tiles;
                if (mahoData) ++holed;
            }
        }
        // native tail (MWMO/MODF mapobjdef creation) is a no-op with wdlState[4] == 0

        if (badSlots)
            WLOG_WARN("adt-split: Cata+ WDL '%s': %u MAOF slot(s) skipped (out of bounds or not MARE)",
                      path, badSlots);
        WLOG_INFO("adt-split: read Cata+ WDL for '%s\\%s' (%u low-detail tiles, %u with holes, "
                  "%u ML* chunks skipped; distant WMOs TODO)",
                  mapPath, mapName, tiles, holed, mlChunks);
        return 1;
    }

    /// SEH shell around the Cata+ WDL reader: a fault on malformed data becomes a logged failure, never
    /// a crash. State stays engine-legal either way. No unwindable locals here (C2712).
    uint32_t ReadCataWdlGuarded(int* wdlState, const char* mapPath, const char* mapName)
    {
        __try
        {
            return ReadCataWdl(wdlState, mapPath, mapName);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    /**
     * @brief Detours CMap::LoadWdl (once per map load, from CMap__Load).
     *
     * Fast path first: a map already cached as NON-split runs the untouched original. Otherwise the WDL
     * header is sniffed (the g_splitMaps cache cannot answer here: the WDL loads BEFORE any tile is
     * requested, so on a fresh map the _tex0 probe has not run yet -- the WDL's own after-MVER tag is
     * the authoritative trigger). A Cata+ WDL is read directly into the stock WDL runtime; everything
     * else runs the byte-identical native body. A Cata+ WDL NEVER falls through to the native parser.
     */
    uint32_t __fastcall hkLoadWdl(int* wdlState, void* edx, const char* mapPath, const char* mapName)
    {
        if (wdlState && mapPath && mapName)
        {
            bool knownMonolithic = false;
            {
                std::string key(mapPath);
                key += '\\';
                key += mapName;
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = g_splitMaps.find(key);
                knownMonolithic = it != g_splitMaps.end() && !it->second;
            }
            if (!knownMonolithic && SniffWdlKindGuarded(mapPath, mapName) == WdlKind::CataPlus)
            {
                const uint32_t result = ReadCataWdlGuarded(wdlState, mapPath, mapName);
                if (result)
                    g_statWdlRead.fetch_add(1, std::memory_order_relaxed);
                else
                {
                    g_statFailures.fetch_add(1, std::memory_order_relaxed);
                    WLOG_ERROR("adt-split: Cata+ WDL read failed for '%s\\%s'; continuing without "
                               "distant terrain", mapPath, mapName);
                }
                return result;
            }
        }
        return g_origLoadWdl(wdlState, edx, mapPath, mapName);
    }
}

namespace wxl::runtime::adtsplit::detail
{
    bool InstallWdl()
    {
        return wxl::hook::Install("AdtSplit_LoadWdl", adt::kLoadWdl, &hkLoadWdl, &g_origLoadWdl);
    }
}
