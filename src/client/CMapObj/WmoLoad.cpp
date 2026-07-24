// Native modern-WMO reader: the tag-driven root + group chunk walkers and the per-batch cull bypass.
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

// MECHANISM (read where it lies, adapt the client)
//
//   Classify   A root buffer is walked once by tag. It is MODERN when it carries no MOTX or carries
//              GFID -- the two facts that make the positional walker desynchronise. Stock roots take
//              the untouched original walker, so a mixed session (stock Azeroth + modern map) is
//              correct. The verdict is recorded per root object and OVERWRITTEN on every root load.
//
//   Root       Our walker fills the same 17 slots the stock one does (offsets/game/WMO.hpp kRootSlots),
//              matching on the tag instead of the position, and reproduces the MOSB emptiness test, the
//              resulting MOGI sky-flag clear, and the MOPT NaN plane repair. Chunks with no 3.3.5 home
//              are counted and skipped -- what a tag-driven walk is for; nothing is rewritten.
//
//   Group      Same shape over kGroupSlots, merging the mandatory and optional walkers into one pass.
//              Presence decides (not the flag bits), which is what makes the modern group's 0x400 LOD bit
//              harmless. MOCV honours the MOHD gate (WmoVertexColor); MOBN + MOBR go to the stock BSP init.
//
//   Parked     What 3.3.5 has no representation for is RETAINED and counted, never rewritten: MODD when
//              the root ships MODI, MLIQ (modern liquid ids), MOVX (32-bit indices). Parking keeps the
//              WMO loadable; each counter is the todo list for the next phase.

#include "config.hpp"
#include "engine/hook/Hook.hpp"
#include "client/CMapObj/WmoNativeShared.hpp"

#include "common/Log.hpp"
#include "game/Binding.hpp"
#include "offsets/game/WMO.hpp"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace off = wxl::offsets::game::wmo;

namespace
{
    using namespace wxl::runtime::wmonative::detail;

    off::Wmo_RootWalkFn  g_origRootWalk  = nullptr;
    off::Wmo_GroupWalkFn g_origGroupWalk = nullptr;
    off::Wmo_CullBatchFn g_origCullBatch = nullptr;

    /// Modern-only chunks: recognised so they are counted rather than reported as unknown.
    bool IsKnownModernOnly(uint32_t tag)
    {
        static const uint32_t kTags[] = {
            off::WmoTag("GFID"), off::WmoTag("MODI"), off::WmoTag("MGI2"), off::WmoTag("MDDI"),
            off::WmoTag("MNLD"), off::WmoTag("MFED"), off::WmoTag("MAVG"), off::WmoTag("MAVD"),
            off::WmoTag("MDDL"), off::WmoTag("MPVD"), off::WmoTag("MOSI"), off::WmoTag("MOUV"),
            off::WmoTag("MOGX"), off::WmoTag("MOBS"), off::WmoTag("MOPB"), off::WmoTag("MNLR"),
            off::WmoTag("MDAL"), off::WmoTag("MFVR"), off::WmoTag("MPVR"), off::WmoTag("MAVR"),
            off::WmoTag("MPY2"), off::WmoTag("MOC2"), off::WmoTag("MOQG"),
        };
        for (uint32_t t : kTags)
            if (t == tag) return true;
        return false;
    }

    /**
     * @brief Classifies a root buffer without writing anything.
     *
     * MODERN means "the positional walker cannot read this": no MOTX (the walker's slot 2, whose
     * absence shifts every later chunk into the wrong slot) or a GFID present (a Midnight root that
     * addresses its groups by FileDataID). Both are decided by presence alone, never by MVER.
     */
    bool ClassifyRootBuffer(uint8_t* base, uint32_t size)
    {
        bool sawMotx = false, sawGfid = false;
        WalkChunks(base, base + size, [&](uint32_t tag, uint8_t*, uint32_t) {
            if (tag == off::WmoTag("MOTX")) sawMotx = true;
            else if (tag == off::WmoTag("GFID")) sawGfid = true;
        });
        return !sawMotx || sawGfid;
    }

    /// Clears the SHOW_SKY bit on every MOGI entry, exactly as the stock walker does when the root
    /// carries no skybox name. The write lands in the file buffer because that is where MOGI lives and
    /// where the client itself performs this clear.
    void ClearMogiSkyFlags(void* root)
    {
        auto* mogi = static_cast<uint8_t*>(GetPtr(root, off::kOffMogiTable));
        if (!mogi) return;
        const uint32_t count = Rd32(Field(root, 0x16C));
        for (uint32_t i = 0; i < count; ++i)
        {
            uint8_t* entry = mogi + i * off::kMogiStride;
            Wr32(entry, Rd32(entry) & ~off::kMogiFlagShowSky);
        }
    }

    /// Repairs portal planes whose distance is NaN, exactly as the stock walker does: normal (0,0,1)
    /// and a distance far outside any map.
    void RepairPortalPlanes(void* root)
    {
        auto* mopt = static_cast<uint8_t*>(GetPtr(root, 0x138));
        if (!mopt) return;
        const uint32_t count = Rd32(Field(root, 0x174));
        for (uint32_t i = 0; i < count; ++i)
        {
            uint8_t* plane = mopt + i * 0x14;
            float distance;
            std::memcpy(&distance, plane + 0x10, 4);
            if (!(distance != distance))    // only a NaN fails this comparison with itself
                continue;
            const float normal[3] = { 0.0f, 0.0f, 1.0f };
            std::memcpy(plane + 0x04, normal, sizeof normal);
            const float repaired = off::kPortalPlaneRepairDistance;
            std::memcpy(plane + 0x10, &repaired, 4);
        }
    }

    /// Tag-driven root fill. Returns false only if the buffer is unusable.
    bool WalkRootModern(void* root)
    {
        auto* base = static_cast<uint8_t*>(GetPtr(root, off::kOffRootBuffer));
        const uint32_t size = Rd32(Field(root, off::kOffRootSize));
        if (!base || size < 12)
            return false;

        for (const auto& slot : off::kRootSlots)
        {
            SetPtr(root, slot.ptrField, nullptr);
            if (slot.countField) SetU32(root, slot.countField, 0);
        }

        uint32_t unknown = 0;
        WalkChunks(base, base + size, [&](uint32_t tag, uint8_t* content, uint32_t chunkSize) {
            for (const auto& slot : off::kRootSlots)
            {
                if (slot.tag != tag) continue;
                SetPtr(root, slot.ptrField, content);
                if (slot.countField)
                    SetU32(root, slot.countField, slot.stride == 1 ? chunkSize : chunkSize / slot.stride);
                return;
            }
            if (tag == off::WmoTag("MVER")) return;
            if (IsKnownModernOnly(tag)) { ++unknown; return; }
            ++unknown;
        });
        g_unknownChunks.fetch_add(unknown, std::memory_order_relaxed);

        // MOSB present but empty means "no skybox", same as absent -- the stock walker collapses both
        // to a null pointer and then clears the sky bit on every group.
        auto* mosb = static_cast<uint8_t*>(GetPtr(root, 0x12C));
        if (mosb && mosb[0] == '\0')
        {
            SetPtr(root, 0x12C, nullptr);
            mosb = nullptr;
        }
        if (!mosb)
            ClearMogiSkyFlags(root);

        RepairPortalPlanes(root);

        // MODD indexes MODN by byte offset. A modern root replaced MODN with MODI (an index table), so
        // there is no name blob to resolve against and the stock doodad spawn would read through a null
        // base. Park the definitions -- the data stays in the buffer for the phase that reads MODI.
        if (!GetPtr(root, 0x150) && Rd32(Field(root, 0x190)) != 0)
        {
            SetU32(root, 0x190, 0);
            SetPtr(root, 0x154, nullptr);
            g_parkedDoodads.fetch_add(1, std::memory_order_relaxed);
        }

        // Four-layer material snapshot. The client's material path stores its two live texture handles
        // INSIDE the record at +0x38/+0x3C -- on a four-layer record those bytes are two of the height
        // texture ids -- so all nine ids are copied out NOW, before material creation runs.
        {
            auto* materials = static_cast<uint8_t*>(GetPtr(root, off::kOffMaterialBase));
            const uint32_t count = Rd32(Field(root, off::kOffMaterialCount));
            // Buffer reads happen OUTSIDE the lock: this runs under the walk's SEH net, and a fault
            // while holding the mutex would abandon it for every later load.
            std::unordered_map<uint32_t, wxl::runtime::wmonative::LayeredMaterial> found;
            for (uint32_t i = 0; materials && i < count; ++i)
            {
                const uint8_t* rec = materials + static_cast<size_t>(i) * off::kMomtStride;
                if (Rd32(rec + off::kOffMomtShader) != off::kShaderIdLayered)
                    continue;
                wxl::runtime::wmonative::LayeredMaterial m{};
                for (int layer = 0; layer < 4; ++layer)
                {
                    m.diffuseFdid[layer] = Rd32(rec + off::kOffMomtLayerDiffuse[layer]);
                    m.heightFdid[layer]  = Rd32(rec + off::kOffMomtLayerHeight[layer]);
                }
                m.envFdid  = Rd32(rec + off::kOffMomtLayerEnv);
                m.tintBgra = Rd32(rec + off::kOffMomtDiffColor);
                m.flags    = Rd32(rec + off::kOffMomtFlags);
                found.emplace(i, m);
            }
            if (!found.empty())
            {
                g_layeredMaterials.fetch_add(static_cast<uint32_t>(found.size()), std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(g_layeredMutex);
                g_layeredByRoot[root] = std::move(found);
            }
        }
        return true;
    }

    void __fastcall hkRootWalk(void* root, void* edx)
    {
        auto* base = static_cast<uint8_t*>(GetPtr(root, off::kOffRootBuffer));
        const uint32_t size = Rd32(Field(root, off::kOffRootSize));

        bool modern = false;
        if (base && size >= 12)
        {
            __try { modern = ClassifyRootBuffer(base, size); }
            __except (EXCEPTION_EXECUTE_HANDLER) { modern = false; }
        }
        RecordRootKind(root, modern);
        // Drop a recycled root slot's previous four-layer snapshot; the modern walk re-registers.
        DropLayeredRoot(root);

        if (!modern)
        {
            g_rootsStock.fetch_add(1, std::memory_order_relaxed);
            g_origRootWalk(root, edx);
            return;
        }

        bool ok = false;
        __try { ok = WalkRootModern(root); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }

        if (!ok)
        {
            // A modern root the stock walker cannot read either: leave every slot null rather than
            // hand it wild pointers. CreateData then allocates zero groups and the WMO stays empty.
            RecordRootKind(root, false);
            WLOG_ERROR("wmo-native: root walk failed, map object left empty (path '%s')",
                       reinterpret_cast<const char*>(Field(root, off::kOffNameInline)));
            return;
        }
        g_rootsModern.fetch_add(1, std::memory_order_relaxed);
    }

    /// Tag-driven group fill, merging the stock mandatory and optional walkers into one pass.
    bool WalkGroupModern(void* group, uint8_t* cursor)
    {
        auto* base = static_cast<uint8_t*>(GetPtr(group, off::kOffGroupBuffer));
        const uint32_t size = Rd32(Field(group, off::kOffGroupSize));
        if (!base || !cursor || size < 0x58)
            return false;

        // The sub-chunks live inside the MOGP payload; its size sits at buffer+0x10 (MVER is 0x0C,
        // then the MOGP tag at +0x0C and its size at +0x10). Clamp to the file for safety.
        uint8_t* end = base + size;
        const uint32_t mogpSize = Rd32(base + 0x10);
        if (mogpSize && mogpSize <= size - 0x14 && base + 0x14 + mogpSize < end)
            end = base + 0x14 + mogpSize;
        if (cursor >= end)
            return false;

        for (const auto& slot : off::kGroupSlots)
        {
            SetPtr(group, slot.ptrField, nullptr);
            if (slot.countField) SetU32(group, slot.countField, 0);
        }
        SetPtr(group, off::kGroupSlotMotv2.ptrField, nullptr);
        SetU32(group, off::kGroupSlotMotv2.countField, 0);
        SetPtr(group, off::kGroupSlotMocv2.ptrField, nullptr);
        SetU32(group, off::kGroupSlotMocv2.countField, 0);
        SetPtr(group, off::kOffGroupMorb, nullptr);

        uint8_t* mobn = nullptr; uint32_t mobnSize = 0;
        uint8_t* mobr = nullptr; uint32_t mobrSize = 0;
        uint32_t motvSeen = 0, mocvSeen = 0, unknown = 0;
        uint8_t* motv[4] = {}; uint32_t motvSize[4] = {}; // all four UV sets (file order = layer order)
        uint8_t* moc2 = nullptr; uint32_t moc2Size = 0;   // per-vertex layer weights (4 B/vertex)

        auto store = [&](const off::ChunkSlot& slot, uint8_t* content, uint32_t chunkSize) {
            SetPtr(group, slot.ptrField, content);
            if (slot.countField)
                SetU32(group, slot.countField, slot.stride == 1 ? chunkSize : chunkSize / slot.stride);
        };

        WalkChunks(cursor, end, [&](uint32_t tag, uint8_t* content, uint32_t chunkSize) {
            // A modern group ships up to FOUR UV sets (the four-layer material's per-layer mappings).
            // The client has slots for two; sets 3/4 have no runtime home, so they are kept as side
            // data for the layered draw path rather than overwriting a slot.
            if (tag == off::WmoTag("MOTV"))
            {
                if (motvSeen == 0)      store(off::kGroupSlots[4], content, chunkSize);
                else if (motvSeen == 1) store(off::kGroupSlotMotv2, content, chunkSize);
                if (motvSeen < 4) { motv[motvSeen] = content; motvSize[motvSeen] = chunkSize; }
                else              ++unknown;
                ++motvSeen;
                return;
            }
            if (tag == off::WmoTag("MOC2")) { moc2 = content; moc2Size = chunkSize; return; }
            if (tag == off::WmoTag("MOCV"))
            {
                if (mocvSeen == 0)      store(off::kGroupSlots[8], content, chunkSize);
                else if (mocvSeen == 1) store(off::kGroupSlotMocv2, content, chunkSize);
                else                    ++unknown;
                ++mocvSeen;
                return;
            }
            if (tag == off::WmoTag("MOBN")) { mobn = content; mobnSize = chunkSize; return; }
            if (tag == off::WmoTag("MOBR")) { mobr = content; mobrSize = chunkSize; return; }
            if (tag == off::WmoTag("MORB")) { SetPtr(group, off::kOffGroupMorb, content); return; }
            if (tag == off::WmoTag("MLIQ")) { g_parkedLiquids.fetch_add(1, std::memory_order_relaxed); return; }
            if (tag == off::WmoTag("MOVX")) { g_parkedIndex32.fetch_add(1, std::memory_order_relaxed); return; }

            for (const auto& slot : off::kGroupSlots)
            {
                if (slot.tag != tag) continue;
                store(slot, content, chunkSize);
                return;
            }
            ++unknown;
        });
        g_unknownChunks.fetch_add(unknown, std::memory_order_relaxed);

        // EXPERIMENTAL UV-orientation probe (see g_uvMode). Rewrites the base UV set in place, or swaps
        // which set feeds the base texture, so the transform that de-rotates modern textures can be found
        // in game. Applied on the file buffer (the client's own working memory, like the MOCV rewrite).
        if (const int mode = g_uvMode.load(std::memory_order_relaxed); mode != 0 && motv[0])
        {
            auto xform = [](uint8_t* uv, uint32_t bytes, int m) {
                for (uint32_t i = 0; i + 8 <= bytes; i += 8)
                {
                    float u, v; std::memcpy(&u, uv + i, 4); std::memcpy(&v, uv + i + 4, 4);
                    float nu = u, nv = v;
                    switch (m)
                    {
                        case 1: nu = v;  nv = u;  break; // swap u/v (reflect across the diagonal)
                        case 2: nu = v;  nv = -u; break; // rotate +90
                        case 3: nu = -v; nv = u;  break; // rotate -90
                        case 4: nu = u;  nv = -v; break; // flip v
                        case 5: nu = -u; nv = v;  break; // flip u
                        default: break;
                    }
                    std::memcpy(uv + i, &nu, 4); std::memcpy(uv + i + 4, &nv, 4);
                }
            };
            if (mode >= 1 && mode <= 5)
                xform(motv[0], motvSize[0], mode);
            else if (mode == 6 && motv[1]) // feed the SECOND UV set to the base texture (t0)
            {
                store(off::kGroupSlots[4], motv[1], motvSize[1]);
                store(off::kGroupSlotMotv2, motv[0], motvSize[0]);
            }
            g_uvTransformed.fetch_add(1, std::memory_order_relaxed);
        }

        // UV routing: base = set 0, file order, always -- the confirmed contract for every material
        // family (Legion ids <= 22 sample set 0 for the base; the four-layer material samples set i per
        // LAYER, served by the layered draw path from the side data below, never by reordering slots).

        // BSP: the stock walker hands the two adjacent chunks straight to CAaBsp, which only copies
        // pointers and counts. Both must be present -- a lone MOBN would leave the face list null.
        if (mobn && mobr)
        {
            wxl::game::Native<off::Wmo_BspInitFn>(off::kBspInit)(
                Field(group, off::kOffGroupBsp), nullptr,
                mobn, mobnSize >> 4, mobr, mobrSize >> 1,
                reinterpret_cast<float*>(Field(group, 0x34)));
        }

        // A modern group can carry geometry with NO texture coordinates at all: collision-only shells (a
        // handful of faces, no MOBA, every batch count 0). 3.3.5 has no such case, so FillVertexVB0 reads
        // MOTV (+0xF0) and MONR (+0xEC) UNCONDITIONALLY once the vertex count is non-zero, and faults on
        // the null. Such a group has nothing to draw, so only the runtime VERTEX count is zeroed: the
        // vertex fill skips it entirely, while MOPY / MOVI / MOVT and the BSP keep their pointers and
        // counts, so collision and portals are untouched. A group WITH batches but no UVs gets an error
        // line rather than silent parking.
        if (Rd32(Field(group, 0x15C)) != 0 && (!GetPtr(group, 0xF0) || !GetPtr(group, 0xEC)))
        {
            if (Rd32(Field(group, 0x16C)) != 0)
                WLOG_ERROR("wmo-native: group has %u batches but no MOTV/MONR; parking its vertices",
                           Rd32(Field(group, 0x16C)));
            SetU32(group, 0x15C, 0);
            g_parkedNoUv.fetch_add(1, std::memory_order_relaxed);
        }

        // MOBA material index: copied into place rather than patched at the read site. Guarded three
        // ways -- the batch must announce the modern form, the destination byte must still be untouched,
        // and the index must be one this client can address (its readers are all `movzx byte`).
        if constexpr (wxl::features::modernWMOSupport)
        {
            auto* batches = static_cast<uint8_t*>(GetPtr(group, off::kGroupSlots[5].ptrField));
            const uint32_t batchCount = Rd32(Field(group, off::kGroupSlots[5].countField));
            if (batches)
            {
                uint32_t moved = 0;
                for (uint32_t i = 0; i < batchCount; ++i)
                {
                    uint8_t* rec = batches + i * off::kMobaStride;
                    if (!(rec[off::kOffMobaFlags] & off::kMobaFlagMaterialModern)) continue;
                    if (rec[off::kOffMobaMaterial] != 0) continue;
                    uint16_t modern;
                    std::memcpy(&modern, rec + off::kOffMobaMaterialModern, 2);
                    if (modern > 0xFF) { g_materialOutOfRange.fetch_add(1, std::memory_order_relaxed); continue; }
                    rec[off::kOffMobaMaterial] = static_cast<uint8_t>(modern);
                    ++moved;
                }
                g_materialIdsMoved.fetch_add(moved, std::memory_order_relaxed);
            }
        }

        // Vertex colour: the whole load-time MOHD-gated contract lives in WmoVertexColor.
        ApplyVertexColor(group);

        // Four-layer side data: registered only when the group ships all four UV sets AND a weight chunk
        // covering every vertex -- exactly the population that carries four-layer materials. Pointers
        // land in the resident group buffer; the layered draw path copies them into its own stream.
        if (const uint32_t vertexCount = Rd32(Field(group, 0x15C));
            moc2 && motvSeen >= 4 && vertexCount != 0 && moc2Size >= vertexCount * 4 &&
            motvSize[0] >= vertexCount * 8 && motvSize[1] >= vertexCount * 8 &&
            motvSize[2] >= vertexCount * 8 && motvSize[3] >= vertexCount * 8)
        {
            std::lock_guard<std::mutex> lock(g_layeredMutex);
            g_layeredByGroup[group] =
                wxl::runtime::wmonative::LayeredGroup{ { motv[0], motv[1], motv[2], motv[3] }, moc2, vertexCount };
            g_layeredGroups.fetch_add(1, std::memory_order_relaxed);
        }
        return true;
    }

    void __fastcall hkGroupWalk(void* group, void* edx, void* cursor)
    {
        // A pooled group slot may be reused by a different group; drop any previous occupant's layered
        // side data before either walk runs (the modern walk re-registers when the data is present).
        DropLayeredGroup(group);

        void* root = GetPtr(group, off::kOffGroupRoot);
        if (!root || !RootIsModern(root))
        {
            g_origGroupWalk(group, edx, cursor);
            return;
        }

        bool ok = false;
        __try { ok = WalkGroupModern(group, static_cast<uint8_t*>(cursor)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }

        if (ok) g_groupsModern.fetch_add(1, std::memory_order_relaxed);
        else    g_groupsFailed.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Per-batch AABB cull, bypassed for modern batches.
     *
     * The stock test reads six i16 at record+0x00. A modern record has zeros there except +0x0A, where
     * the material index lives -- so every modern batch claims a ZERO-VOLUME box at the group origin and
     * is rejected as soon as the camera comes close enough for that single point to leave the frustum.
     * That is exactly the "the nearer I get, the more of the building disappears" symptom.
     *
     * A modern file simply does not ship this box (Legion culls per group and on the GPU). Inventing one
     * inside the record is not an option -- it would overwrite the material index at +0x0A -- so the
     * per-batch REFINEMENT is skipped for modern batches: they are reported visible, and group-level
     * frustum, portal and horizon culling still decide. The test is deliberately narrow: the announce bit
     * AND an all-zero min/max.xy, so a stock batch is never mistaken for modern.
     */
    char __cdecl hkCullBatch(void* mobaRecord)
    {
        if (mobaRecord)
        {
            const auto* rec = static_cast<const uint8_t*>(mobaRecord);
            if ((rec[off::kOffMobaFlags] & off::kMobaFlagMaterialModern) != 0 &&
                Rd32(rec) == 0 && Rd32(rec + 4) == 0 && rec[8] == 0 && rec[9] == 0)
            {
                g_batchCullBypassed.fetch_add(1, std::memory_order_relaxed);
                g_curBatch = mobaRecord;    // the effect bind that follows belongs to this batch
                return 0;   // 0 = keep this batch
            }
        }
        return g_origCullBatch(mobaRecord);
    }
}

namespace wxl::runtime::wmonative::detail
{
    bool InstallRootWalk()
    {
        return wxl::hook::Install("WmoNative_RootWalk", off::kRootWalk, &hkRootWalk, &g_origRootWalk);
    }

    bool InstallGroupWalk()
    {
        return wxl::hook::Install("WmoNative_GroupWalk", off::kGroupWalk, &hkGroupWalk, &g_origGroupWalk);
    }

    bool InstallCull()
    {
        return wxl::hook::Install("WmoNative_CullBatch", off::kCullBatch, &hkCullBatch, &g_origCullBatch);
    }
}
