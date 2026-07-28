// Native split-ADT reader: the feature entry -- the async 3-read chain, the tile detours, the query surface.
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

// MECHANISM (direct-fill, no monolithic merge). This unit owns the tile lifetime: CMapArea::Load arms a
// 3-read async chain (root -> _tex0 -> _obj0, each into a resident buffer, exactly one async in flight at
// area+0x70 so the stock purge/cancel path is untouched); the completion advances the chain and, after
// the third read, parses (AdtParse) + hands the area to the UNCHANGED stock CMapArea::Create. Per chunk,
// ProcessIffChunks direct-assigns the sub-chunk slots into the resident buffers. The destructor detour
// releases the tile's side record AFTER the original body, when every chunk is already purged.
// Detection lives in AdtDetect, the trio parse/fixups in AdtParse, the Cata+ WDL in AdtWdl, and the
// split-tile texture manager + height slots in AdtTexture.

#include "config.hpp"
#include "engine/hook/Hook.hpp"
#include "engine/hook/Registry.hpp"
#include "engine/events/Event.hpp"
#include "client/CMapArea/AdtSplit.hpp"
#include "client/CMapArea/AdtSplitInternal.hpp"

#include "common/Log.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

namespace
{
    using namespace wxl::runtime::adtsplit::detail;
    namespace ev = wxl::events;

    adt::TileAreaLoadFn           g_origTileAreaLoad    = nullptr;
    adt::ChunkProcessIffChunksFn  g_origProcessIff      = nullptr;
    adt::TileAreaDestroyFn        g_origTileAreaDestroy = nullptr;

    // ---------------------------------------------------------------- the async chain
    void __cdecl SplitStageComplete(void* areaCtx); // fwd

    /**
     * @brief Arms one async whole-file read for a stage: open -> size -> AllocRawAreaData ->
     *        CAsyncObject{file,buf,size,ctx=area,cb} -> area+0x70/+0x6C -> enqueue.
     *
     * Mirrors the native CMapArea::Load body field-for-field so the stock purge guard/cancel path treats
     * the read exactly like the stock single read. Returns false (nothing armed, area fields untouched)
     * when the stage's file is absent or empty.
     */
    bool StartStage(SplitTile& t, Stage stage)
    {
        const char* name = (stage == Stage::Tex) ? t.texName : t.objName;
        void* file = OpenFile(name);
        if (!file) return false;
        const uint32_t size = SizeOfFile(file);
        if (size == 0 || size == 0xFFFFFFFFu) { CloseFile(file); return false; }
        uint8_t* buf = AllocRaw(size);
        if (!buf) { CloseFile(file); return false; }
        void* async = AsyncAlloc();
        if (!async) { FreeRaw(buf, size); CloseFile(file); return false; }

        if (stage == Stage::Tex) { t.texBuf = buf; t.texSize = size; t.texOwned = true; }
        else                     { t.objBuf = buf; t.objSize = size; t.objOwned = true; }

        At<void*>(async, wld::kOffAsyncFile)     = file;
        At<void*>(async, wld::kOffAsyncBuffer)   = buf;
        At<uint32_t>(async, wld::kOffAsyncSize)  = size;
        At<void*>(async, wld::kOffAsyncCtx)      = t.area;
        At<void*>(async, wld::kOffAsyncCallback) = reinterpret_cast<void*>(&SplitStageComplete);

        t.stage    = stage;
        t.curAsync = async;
        At<void*>(t.area, adt::kOffTileAsyncRead)  = async;
        At<void*>(t.area, adt::kOffTileFileHandle) = file;
        AsyncEnqueue(async);
        return true;
    }

    /**
     * @brief Final step, on the main thread in the exact slot the stock parser would run: parse + patch,
     *        then hand the area to the UNCHANGED stock CMapArea::Create, then replicate the native
     *        completion epilogue (async retire, zero +0x70/+0x6C).
     */
    void FinalizeTile(SplitTile& t)
    {
        void* area = t.area;
        const bool parsed = ParseAndPatchGuarded(&t);
        if (!parsed)
        {
            g_statFailures.fetch_add(1, std::memory_order_relaxed);
            WLOG_ERROR("adt-split: tile %d_%d parse FAILED; running the stock parser over the raw root "
                       "(native corrupt-data behaviour)", t.tileFirst, t.tileSecond);
        }
        t.complete = parsed; // gate the ProcessIffChunks direct-fill on a good index only
        wxl::game::Native<adt::TileAreaCreateFn>(adt::kTileAreaCreate)(area, nullptr);

        if (t.curAsync) { AsyncRetire(t.curAsync); t.curAsync = nullptr; } // closes the last file
        At<void*>(area, adt::kOffTileAsyncRead)  = nullptr;
        At<void*>(area, adt::kOffTileFileHandle) = nullptr;
        t.stage = Stage::Done;

        if (parsed)
        {
            g_statTilesLoaded.fetch_add(1, std::memory_order_relaxed);
            if (t.mtxp) g_statMtxpTiles.fetch_add(1, std::memory_order_relaxed);
            g_statMclvChunks.fetch_add(t.mclvChunks, std::memory_order_relaxed);
            g_statHoleChunks.fetch_add(t.hiResHoleChunks, std::memory_order_relaxed);
            WLOG_INFO("adt-split: tile %d_%d loaded (root %u B, tex %u B, obj %u B, %u chunks, mcrf %zu B)",
                      t.tileFirst, t.tileSecond, t.rootSize, t.texSize, t.objSize, t.chunkCount,
                      t.mcrfPool.size());
            ev::AdtSplitTileLoadArgs a{ t.tileFirst, t.tileSecond, t.rootSize, t.texSize, t.objSize,
                                        t.chunkCount };
            ev::Emit(ev::Event::OnAdtSplitTileLoad, &a);
        }
    }

    /**
     * @brief Main-thread completion for every stage of the 3-read chain (installed as async+0x10,
     *        invoked by the stock drain as callback(ctx=area)).
     *
     * Advances root -> _tex0 -> _obj0 -> finalize. A missing _tex0/_obj0 skips forward (the tile then
     * renders with whatever the trio provided). A tile torn down mid-chain has its async cancelled by the
     * stock purge and its registry entry removed by the destructor detour, so a stale invocation finds no
     * record and touches nothing.
     */
    void __cdecl SplitStageComplete(void* areaCtx)
    {
        SplitTile* t = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            t = FindTileLocked(areaCtx);
        }
        if (!t) return;

        switch (t->stage)
        {
        case Stage::Root:
            if (t->curAsync) { AsyncRetire(t->curAsync); t->curAsync = nullptr; } // closes root file
            At<void*>(t->area, adt::kOffTileAsyncRead)  = nullptr;
            At<void*>(t->area, adt::kOffTileFileHandle) = nullptr;
            if (!StartStage(*t, Stage::Tex) && !StartStage(*t, Stage::Obj))
                FinalizeTile(*t);
            break;
        case Stage::Tex:
            if (t->curAsync) { AsyncRetire(t->curAsync); t->curAsync = nullptr; } // closes tex file
            At<void*>(t->area, adt::kOffTileAsyncRead)  = nullptr;
            At<void*>(t->area, adt::kOffTileFileHandle) = nullptr;
            if (!StartStage(*t, Stage::Obj))
                FinalizeTile(*t);
            break;
        case Stage::Obj:
            // native ordering: the parser runs BEFORE the async retire (+0x70 stays armed under it)
            FinalizeTile(*t);
            break;
        default:
            break;
        }
    }

    // ---------------------------------------------------------------- tile detours
    /**
     * @brief Detours CMapArea::Load (the per-tile open + async-read arm).
     *
     * Split tile: arms the 3-read chain (root first, stock-shaped) and returns without the original.
     * Non-split tile, unknown name, or any arming failure: the untouched native body runs -- the stock
     * path stays byte-identical for classic maps.
     */
    void __fastcall hkTileAreaLoad(void* area, void* edx, const char* filename)
    {
        if (filename && area)
        {
            std::string key;
            if (IsSplitTileName(filename, key))
            {
                // The alpha-layout flag lives in map-header state that is rebuilt whenever a world
                // loads, so every tile re-arms it (idempotent) instead of it being set once when the
                // map is first recognised. This runs before the tile's coverage is ever sized.
                EnsureAlphaLayoutFlag();

                // stage 1 (root), shaped exactly like the native body
                void* file = OpenFile(filename);
                if (file)
                {
                    const uint32_t size = SizeOfFile(file);
                    uint8_t* buf   = (size && size != 0xFFFFFFFFu) ? AllocRaw(size) : nullptr;
                    void*    async = buf ? AsyncAlloc() : nullptr;
                    if (async)
                    {
                        auto tile = std::make_unique<SplitTile>();
                        tile->area       = area;
                        tile->tileFirst  = At<int>(area, adt::kOffTileIdxFirst);
                        tile->tileSecond = At<int>(area, adt::kOffTileIdxSecond);
                        const size_t base = std::strlen(filename) - 4;
                        std::snprintf(tile->texName, sizeof tile->texName, "%.*s_tex0.adt",
                                      static_cast<int>(base), filename);
                        std::snprintf(tile->objName, sizeof tile->objName, "%.*s_obj0.adt",
                                      static_cast<int>(base), filename);
                        tile->rootBuf  = buf;
                        tile->rootSize = size;
                        tile->stage    = Stage::Root;
                        tile->curAsync = async;

                        At<void*>(async, wld::kOffAsyncFile)     = file;
                        At<void*>(async, wld::kOffAsyncBuffer)   = buf;
                        At<uint32_t>(async, wld::kOffAsyncSize)  = size;
                        At<void*>(async, wld::kOffAsyncCtx)      = area;
                        At<void*>(async, wld::kOffAsyncCallback) = reinterpret_cast<void*>(&SplitStageComplete);

                        At<void*>(area, adt::kOffTileFileHandle)   = file;
                        At<uint32_t>(area, adt::kOffTileFileSize)  = size;
                        At<void*>(area, adt::kOffTileFileBuffer)   = buf;
                        At<void*>(area, adt::kOffTileAsyncRead)    = async;
                        {
                            std::lock_guard<std::mutex> lock(g_mutex);
                            // Records are keyed by the loading tile object, and that address is
                            // recycled across world loads. A leftover here means a teardown bypassed
                            // the teardown seam, so the record is dropped rather than inherited by the
                            // new tile -- its raw buffers are left alone, since ownership of them can
                            // no longer be established.
                            if (g_tiles.erase(area) != 0)
                            {
                                g_statTilesResident.fetch_sub(1, std::memory_order_relaxed);
                                WLOG_WARN("adt-split: stale tile record found on a recycled tile slot, dropped");
                            }
                            g_tiles[area] = std::move(tile);
                        }
                        g_statTilesResident.fetch_add(1, std::memory_order_relaxed);
                        AsyncEnqueue(async);
                        return;
                    }
                    if (buf) FreeRaw(buf, size);
                    CloseFile(file);
                }
                g_statFailures.fetch_add(1, std::memory_order_relaxed);
                WLOG_WARN("adt-split: split arming failed for '%s', falling back to the native load", filename);
            }
        }
        g_origTileAreaLoad(area, edx, filename);
    }

    /**
     * @brief Detours CMapChunk::ProcessIffChunks (the sequential sub-chunk walk).
     *
     * For a chunk of a completed split tile the walk is REPLACED by direct pointer assignment: the
     * chunk+0x11C..+0x13C sub-chunk slots aim straight into the three resident split buffers (and the
     * tiny MCRF pool), zero payload copies. Non-split chunks run the untouched original.
     */
    void __fastcall hkProcessIffChunks(void* chunk, void* edx, int firstBuild)
    {
        // Fast path: with no split tile resident (every classic 3.3.5 map, always) the stock walk runs
        // with only a single relaxed atomic load added -- no owner-link deref, no lock, no hash lookup.
        if (g_statTilesResident.load(std::memory_order_relaxed) == 0)
        {
            g_origProcessIff(chunk, edx, firstBuild);
            return;
        }

        // chunk -> owning area, the engine's own link arithmetic: *((link & ~1) + 8)
        void* area = nullptr;
        const uint32_t link = At<uint32_t>(chunk, adt::kOffChunkTexOwnerSrc);
        if (link != 0 && (link & 1u) == 0)
            area = *reinterpret_cast<void**>(static_cast<uintptr_t>(link) + 8);

        const ChunkFill* f = nullptr;
        if (area)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            SplitTile* t = FindTileLocked(area);
            if (t && t->complete)
            {
                const int lx = At<int>(chunk, adt::kOffMapChunkIndexX);
                const int ly = At<int>(chunk, adt::kOffMapChunkIndexY);
                const uint32_t idx = static_cast<uint32_t>(ly) * 16u + static_cast<uint32_t>(lx);
                uint8_t* raw = At<uint8_t*>(chunk, adt::kOffChunkRawMcnk);
                if (idx < 256 && t->chunks[idx].rootMcnkHdr == raw)
                    f = &t->chunks[idx];
                else
                    WLOG_WARN("adt-split: chunk (%d,%d) raw MCNK mismatch, using stock walk", lx, ly);
            }
        }
        if (!f)
        {
            g_origProcessIff(chunk, edx, firstBuild);
            return;
        }

        (void)firstBuild;
        uint8_t* raw = At<uint8_t*>(chunk, adt::kOffChunkRawMcnk);
        At<void*>(chunk, adt::kOffChunkMcnkHeader) = raw + 8;                 // 0x80-byte SMChunk header
        At<void*>(chunk, adt::kOffChunkMcvt) = f->mcvt;
        At<void*>(chunk, adt::kOffChunkMccv) = f->mccv;
        At<void*>(chunk, adt::kOffChunkMcnr) = f->mcnr;
        At<void*>(chunk, adt::kOffChunkMcsh) = f->mcsh;
        At<void*>(chunk, adt::kOffChunkMcly) = f->mcly;
        At<void*>(chunk, adt::kOffChunkMcal) = f->mcal;
        At<void*>(chunk, adt::kOffChunkMcrf) = f->mcrf;
        At<void*>(chunk, adt::kOffChunkMclq) = (f->mclq && f->mclqSize) ? f->mclq : nullptr;
        At<void*>(chunk, adt::kOffChunkMcse) = (f->mcse && f->mcseSize >= 0x1C) ? f->mcse : nullptr;
        g_statChunksFilled.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Detours the CMapArea destructor (tile teardown, frees area+0x80).
     *
     * Releases the tile's side record AFTER the original body -- by then every chunk of the tile is
     * already purged, so no CMapChunk pointer into the _tex0/_obj0 buffers can outlive them. Handles the
     * purge-cancel ownership transfer: a DEFERRED cancel (read in service) hands the in-flight stage's
     * buffer to the engine's deferred cleanup and zeroes area+0x80 -- in that case the transferred buffer
     * is NOT freed here, and the orphaned root buffer IS.
     */
    void __fastcall hkTileAreaDestroy(void* area)
    {
        std::unique_ptr<SplitTile> t;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_tiles.find(area);
            if (it != g_tiles.end())
            {
                t = std::move(it->second);
                g_tiles.erase(it);
            }
        }

        bool freeRootAfter = false;
        if (t)
        {
            if (!t->complete)
            {
                const bool deferredCancel =
                    At<void*>(area, adt::kOffTileFileBuffer) == nullptr && t->rootBuf != nullptr;
                if (deferredCancel)
                {
                    if (t->stage == Stage::Tex)      t->texOwned = false; // deferred cleanup frees it
                    else if (t->stage == Stage::Obj) t->objOwned = false;
                    if (t->stage != Stage::Root)     freeRootAfter = true; // +0x80 zeroed, root orphaned
                }
            }
            // Defensive: a teardown path that skipped the purge cancel leaves our async armed; retire it
            // (spin-waits an in-service read, unlinks the completion, closes the file) first.
            void* pending = At<void*>(area, adt::kOffTileAsyncRead);
            if (pending && pending == t->curAsync)
            {
                AsyncRetire(pending);
                t->curAsync = nullptr;
                At<void*>(area, adt::kOffTileAsyncRead)  = nullptr;
                At<void*>(area, adt::kOffTileFileHandle) = nullptr;
            }
        }

        g_origTileAreaDestroy(area);

        if (t)
        {
            // Height-blend "_h" handles are module-owned (the stock destructor only frees the tile's own
            // area+0x60 diffuse slots); release each once, after every chunk of the tile purged.
            for (const SplitTile::HeightSlot& hs : t->heightSlots)
                if (hs.owned && hs.tex)
                    wxl::game::Native<adt::TextureReleaseFn>(adt::kTextureRelease)(hs.tex);
            if (t->texOwned && t->texBuf) FreeRaw(t->texBuf, t->texSize);
            if (t->objOwned && t->objBuf) FreeRaw(t->objBuf, t->objSize);
            if (freeRootAfter)            FreeRaw(t->rootBuf, t->rootSize);
            g_statTilesResident.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    /**
     * @brief Installs the split-ADT detours: the load seam, per-chunk fill seam, teardown seam, and
     *        (delegated) the Cata+ WDL guard and the split-tile texture manager.
     */
    bool InstallAdtSplit()
    {
        wxl::hook::Install("AdtSplit_TileAreaLoad", adt::kTileAreaLoad,
                           &hkTileAreaLoad, &g_origTileAreaLoad);
        wxl::hook::Install("AdtSplit_ProcessIffChunks", adt::kChunkProcessIffChunks,
                           &hkProcessIffChunks, &g_origProcessIff);
        wxl::hook::Install("AdtSplit_TileAreaDestroy", adt::kTileAreaDestroy,
                           &hkTileAreaDestroy, &g_origTileAreaDestroy);
        InstallWdl();
        InstallTextures();
        return true;
    }
}

// ---------------------------------------------------------------- public query surface
namespace wxl::runtime::adtsplit
{
    Stats GetStats()
    {
        Stats s{};
        s.splitMapsDetected = detail::g_statSplitMaps.load(std::memory_order_relaxed);
        s.tilesLoaded       = detail::g_statTilesLoaded.load(std::memory_order_relaxed);
        s.tilesResident     = detail::g_statTilesResident.load(std::memory_order_relaxed);
        s.chunksFilled      = detail::g_statChunksFilled.load(std::memory_order_relaxed);
        s.mcrfBytes         = detail::g_statMcrfBytes.load(std::memory_order_relaxed);
        s.parkedMtxpTiles   = detail::g_statMtxpTiles.load(std::memory_order_relaxed);
        s.parkedMclvChunks  = detail::g_statMclvChunks.load(std::memory_order_relaxed);
        s.parkedHoleChunks  = detail::g_statHoleChunks.load(std::memory_order_relaxed);
        s.loadFailures      = detail::g_statFailures.load(std::memory_order_relaxed);
        s.wdlRead           = detail::g_statWdlRead.load(std::memory_order_relaxed);
        s.heightTexLoaded   = detail::g_statHeightTex.load(std::memory_order_relaxed);
        s.doodadModels      = detail::g_statDoodadModels.load(std::memory_order_relaxed);
        s.mapObjects        = detail::g_statMapObjects.load(std::memory_order_relaxed);
        s.mapObjectsDropped = detail::g_statMapObjectsDropped.load(std::memory_order_relaxed);
        s.liquidLayers      = detail::g_statLiquidLayers.load(std::memory_order_relaxed);
        s.liquidDegraded    = detail::g_statLiquidDegraded.load(std::memory_order_relaxed);
        return s;
    }

    uint32_t ResidentTilesRelaxed()
    {
        return detail::g_statTilesResident.load(std::memory_order_relaxed);
    }

    int IsSplitMap()
    {
        // Compose the same "<dir>\<name>" prefix the tile-name keying uses from the live loader globals
        // (phasing swaps them per load, but the base map's key is what a user asks about).
        const char* dir  = reinterpret_cast<const char*>(wxl::offsets::game::world::kMapDirStr);
        const char* name = reinterpret_cast<const char*>(wxl::offsets::game::world::kMapNameStr);
        if (!dir[0] || !name[0]) return -1;
        std::string key(dir);
        key += '\\';
        key += name;
        std::lock_guard<std::mutex> lock(detail::g_mutex);
        auto it = detail::g_splitMaps.find(key);
        if (it == detail::g_splitMaps.end()) return -1;
        return it->second ? 1 : 0;
    }

    namespace
    {
        void FillStatus(const detail::SplitTile& t, TileStatus& out)
        {
            out.tileFirst       = t.tileFirst;
            out.tileSecond      = t.tileSecond;
            out.rootSize        = t.rootSize;
            out.texSize         = t.texSize;
            out.objSize         = t.objSize;
            out.chunkCount      = t.chunkCount;
            out.complete        = t.complete;
            out.hasMtxp         = t.mtxp != nullptr;
            out.mclvChunks      = t.mclvChunks;
            out.hiResHoleChunks = t.hiResHoleChunks;
        }
    }

    uint32_t ResidentTileCount()
    {
        std::lock_guard<std::mutex> lock(detail::g_mutex);
        return static_cast<uint32_t>(detail::g_tiles.size());
    }

    bool GetResidentTile(uint32_t index, TileStatus& out)
    {
        std::lock_guard<std::mutex> lock(detail::g_mutex);
        uint32_t i = 0;
        for (const auto& kv : detail::g_tiles)
        {
            if (i++ == index)
            {
                FillStatus(*kv.second, out);
                return true;
            }
        }
        return false;
    }

    bool FindTile(int tileFirst, int tileSecond, TileStatus& out)
    {
        std::lock_guard<std::mutex> lock(detail::g_mutex);
        for (const auto& kv : detail::g_tiles)
        {
            if (kv.second->tileFirst == tileFirst && kv.second->tileSecond == tileSecond)
            {
                FillStatus(*kv.second, out);
                return true;
            }
        }
        return false;
    }
}

WXL_REGISTER_FEATURE("adt-split", wxl::features::modernADTSupport, InstallAdtSplit)
