// Native split-ADT reader: internal contract shared across the reader's translation units.
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

#pragma once

// The reader is split by responsibility (state / detect / parse / async / wdl / texture) but is ONE
// subsystem: every stage mutates the same per-tile SplitTile record, keyed by CMapArea* in one map.
// That model, the raw-byte helpers, the by-address native shims, and the telemetry counters live here
// (state defined once in AdtState.cpp) so each stage TU shares one view of a tile's lifetime.

#include "game/Binding.hpp"
#include "offsets/engine/Io.hpp"
#include "offsets/game/ADT.hpp"
#include "offsets/game/World.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace wxl::runtime::adtsplit::detail
{
    namespace adt = wxl::offsets::game::adt;
    namespace wld = wxl::offsets::game::world;
    namespace io  = wxl::offsets::engine::io;

    // ---------------------------------------------------------------- map-header bits
    // Two bits of the live map header the reader cares about: the dataset says it has per-layer
    // height texturing, and the terrain build sizes layer coverage as one byte per texel.
    constexpr uint32_t kMapHeightTexturing = 0x80u;
    constexpr uint32_t kMapWideAlpha       = 0x4u;

    // ---------------------------------------------------------------- raw byte helpers
    inline uint16_t Rd16(const void* p)       { uint16_t v; std::memcpy(&v, p, 2); return v; }
    inline uint32_t Rd32(const void* p)       { uint32_t v; std::memcpy(&v, p, 4); return v; }
    inline uint64_t Rd64(const void* p)       { uint64_t v; std::memcpy(&v, p, 8); return v; }
    inline void     Wr32(void* p, uint32_t v) { std::memcpy(p, &v, 4); }
    inline void     Wr16(void* p, uint16_t v) { std::memcpy(p, &v, 2); }

    /// FourCC as the client compares it: chunk tags are stored byte-reversed on disk, so the
    /// little-endian dword read equals ('M'<<24)|('C'<<16)|... for an "MC.." chunk.
    constexpr uint32_t FourCC(const char (&s)[5])
    {
        return (uint32_t(uint8_t(s[0])) << 24) | (uint32_t(uint8_t(s[1])) << 16) |
               (uint32_t(uint8_t(s[2])) << 8)  |  uint32_t(uint8_t(s[3]));
    }

    /// True when the 4 bytes at p read back as an "MC.." sub-chunk tag (disk order is reversed).
    inline bool LooksLikeSubTag(const uint8_t* p) { return p[3] == 'M' && p[2] == 'C'; }

    template <class T>
    inline T& At(void* base, size_t off) { return *reinterpret_cast<T*>(static_cast<uint8_t*>(base) + off); }

    /// Generic bounds-checked top-level chunk walk; cb(tag, chunkHeaderPtr, dataSize).
    template <class Fn>
    void WalkTop(uint8_t* buf, uint32_t size, Fn&& cb)
    {
        uint32_t off = 0;
        while (off + 8 <= size)
        {
            const uint32_t tag = Rd32(buf + off);
            const uint32_t sz  = Rd32(buf + off + 4);
            if (sz > size - off - 8) break;
            cb(tag, buf + off, sz);
            off += 8 + sz;
        }
    }

    // ---------------------------------------------------------------- native call shims
    // The storage entry points are called BY ADDRESS so the calls land in the installed detours
    // (StorageHook serves host files through them), same trick Phasing uses for its WDT probe.
    inline void* OpenFile(const char* name)
    {
        void* h = nullptr;
        if (!reinterpret_cast<io::Storage_FileOpenFn>(io::kFileOpen)(nullptr, name, 0, &h))
            return nullptr;
        return h;
    }
    inline uint32_t SizeOfFile(void* h) { return reinterpret_cast<io::Storage_FileSizeFn>(io::kFileSize)(h, nullptr); }
    inline void     CloseFile(void* h)  { reinterpret_cast<io::Storage_FileCloseFn>(io::kFileClose)(h); }
    inline bool     ReadBytes(void* h, void* dst, uint32_t len)
    {
        return reinterpret_cast<io::Storage_FileReadFn>(io::kFileRead)(h, dst, len, nullptr, nullptr, 0) != 0;
    }

    inline uint8_t* AllocRaw(uint32_t size)
    {
        return static_cast<uint8_t*>(wxl::game::Native<adt::AllocRawAreaDataFn>(adt::kAllocRawAreaData)(size));
    }
    inline void FreeRaw(void* p, uint32_t size)
    {
        wxl::game::Native<adt::FreeRawAreaDataFn>(adt::kFreeRawAreaData)(p, size);
    }
    inline void* AsyncAlloc()
    {
        return wxl::game::Native<wld::AsyncFileReadAllocObjectFn>(wld::kAsyncFileReadAllocObject)();
    }
    // Enqueue by address: streaming's round-robin detour on AsyncFileReadObject receives the call.
    inline void AsyncEnqueue(void* obj)
    {
        reinterpret_cast<wld::AsyncFileReadObjectFn>(wld::kAsyncFileReadObject)(obj, 0);
    }
    // AsyncFileReadDestroyObject: spin-waits an in-service read, unlinks a queued completion so it can
    // never run, closes the object's file handle and recycles the node.
    inline void AsyncRetire(void* obj)
    {
        reinterpret_cast<wld::AsyncDestroyFn>(wld::kAsyncDestroy)(obj);
    }

    // ---------------------------------------------------------------- per-tile model
    enum class Stage : int { Root = 0, Tex = 1, Obj = 2, Done = 3 };

    /// Per-chunk direct-fill index: every pointer aims INTO one of the three resident buffers, except
    /// mcrf which may aim into the tile's small MCRF concat pool.
    struct ChunkFill
    {
        uint8_t* rootMcnkHdr = nullptr; // raw MCNK (tag+size) in the root buffer
        // root-file sub-chunks
        uint8_t* mcvt = nullptr;
        uint8_t* mccv = nullptr;
        uint8_t* mcnr = nullptr;
        uint8_t* mclq = nullptr; uint32_t mclqSize = 0;
        uint8_t* mcse = nullptr; uint32_t mcseSize = 0;
        // _tex0 sub-chunks (header-less MCNK, correspondence by order)
        uint8_t* mcly = nullptr; uint32_t mclySize = 0;
        uint8_t* mcal = nullptr; uint32_t mcalSize = 0;
        uint8_t* mcsh = nullptr; uint32_t mcshSize = 0;
        // _obj0 sub-chunks
        uint8_t* mcrd = nullptr; uint32_t mcrdSize = 0;
        uint8_t* mcrw = nullptr; uint32_t mcrwSize = 0;
        // materialized fixup (MCRD data ‖ MCRW data) or a direct alias when one side is empty
        uint8_t* mcrf = nullptr;
        uint32_t nDoodadRefs = 0;
        uint32_t nMapObjRefs = 0;
        // parked (no 3.3.5 runtime home; retained for later features)
        uint8_t* mclv = nullptr;
        uint64_t hiResHoles = 0;
        bool     hadHiResHoles = false;
    };

    /// Per-tile side record. Owns the _tex0/_obj0 buffers (root is owned by the area at +0x80) and every
    /// small fixup allocation the direct-filled chunks point at. Lifetime: created in the CMapArea::Load
    /// detour, released in the CMapArea::destructor detour AFTER the original body -- i.e. after every
    /// chunk of the tile is already purged, so nothing can dangle.
    struct SplitTile
    {
        void*    area = nullptr;
        int      tileFirst = 0, tileSecond = 0;
        char     texName[300] = { 0 };
        char     objName[300] = { 0 };
        Stage    stage    = Stage::Root;
        bool     complete = false;
        void*    curAsync = nullptr;

        uint8_t* rootBuf = nullptr; uint32_t rootSize = 0; // aliased by area+0x80/+0x84
        uint8_t* texBuf  = nullptr; uint32_t texSize  = 0; bool texOwned = false;
        uint8_t* objBuf  = nullptr; uint32_t objSize  = 0; bool objOwned = false;

        ChunkFill chunks[256];
        uint32_t  chunkCount = 0;

        std::vector<uint8_t> mcin;       // synthesized MCIN chunk (8-byte header + 256 x 16 B)
        std::vector<uint8_t> mcrfPool;   // concatenated MCRD‖MCRW payloads (chunks alias into it)
        std::vector<uint8_t> synthEmpty; // synthesized empty top-level chunks for absent tables
        // Real MMDX/MMID name-table chunks synthesized from the doodads' MDDF FileDataID nameIds
        // (resolved via ModelFilePath). Each is a full {tag,size,data} chunk; MHDR points at them and
        // the (in-place rewritten) MDDF entries index MMID. Owned for the tile lifetime.
        std::vector<uint8_t> mmdxChunk;  // "MMDX" + NUL-terminated model paths
        std::vector<uint8_t> mmidChunk;  // "MMID" + u32 offsets into the MMDX payload
        // Same treatment for placed map objects: MODF nameIds carry a FileDataID under entry flag 0x8
        // and a Legion+ _obj0 ships no MWMO/MWID at all. Unlike a doodad, a map object that fails to
        // resolve is FATAL (the client hashes the name straight away, and CMap::SafeOpen has no
        // ErrorCube equivalent), so an unresolved entry is instead struck from every MCRW ref list --
        // see modfDead below. Nothing is removed from MODF itself: the entry stays, simply unreferenced.
        std::vector<uint8_t> mwmoChunk;  // "MWMO" + NUL-terminated map-object paths
        std::vector<uint8_t> mwidChunk;  // "MWID" + u32 offsets into the MWMO payload
        std::vector<uint8_t> modfDead;   // one flag per MODF entry: 1 = unresolved, never reference it
        std::vector<uint8_t> mcrwFiltered; // rebuilt MCRW payloads when modfDead removed anything

        // Texture FileDataIDs read from _tex0 MDID (diffuse), indexed by MCLY.textureId. Legion+ tiles
        // carry no MTEX; these are how the terrain layers name their textures. The tex manager detour
        // resolves each through TextureFilePath.db2 into texNameBlob (NUL-terminated client paths, in
        // MDID order) and feeds THAT to the stock CMapArea::LoadTextures -- the slots then point into
        // this persistent blob for the tile lifetime (freed with the SplitTile).
        std::vector<uint32_t> mdid;
        std::vector<char>     texNameBlob;

        // parked tile-level foreigners
        uint8_t* mtxp = nullptr; uint32_t mtxpSize = 0;    // into the resident _tex0 buffer
        uint8_t  mampValue = 0;                            // MHDR+0x30 / MAMP byte (zeroed for 3.3.5)
        uint32_t mclvChunks = 0, hiResHoleChunks = 0;

        // Height-blend inputs. mhid mirrors mdid (index-aligned height-texture FileDataIDs); mtxfData/
        // mtexData park the _tex0 MTXF flag table and MTEX name blob (both point into the resident
        // _tex0 buffer) for the MTXP-default flag fallback and the name-based "_h.blp" derivation on
        // tiles without FileDataIDs.
        std::vector<uint32_t> mhid;
        uint8_t* mtxfData = nullptr; uint32_t mtxfSize = 0;
        uint8_t* mtexData = nullptr; uint32_t mtexSizeParked = 0;
        /// Per-texture height slot, MTXP/MDID order. tex==null means "solid white" (default params,
        /// flag 0x1, or an unresolvable file). owned marks handles this record must release. exp is the
        /// UV-tiling exponent from the record's flags bits 4..7 (applies to the diffuse layer whether or
        /// not a height texture exists).
        struct HeightSlot
        {
            void* tex = nullptr; bool owned = false;
            float scale = 0.0f; float offset = 1.0f; uint8_t exp = 0;
        };
        std::vector<HeightSlot> heightSlots;
        bool heightBuilt = false;
    };

    // ---------------------------------------------------------------- shared state
    extern std::mutex g_mutex; // guards the two maps below (loads are main-thread; snapshots read them)
    extern std::unordered_map<void*, std::unique_ptr<SplitTile>> g_tiles;   // key: CMapArea*
    extern std::unordered_map<std::string, bool> g_splitMaps;               // key: "<dir>\<name>" prefix

    extern std::atomic<uint32_t> g_statSplitMaps, g_statTilesLoaded, g_statTilesResident,
        g_statChunksFilled, g_statMcrfBytes, g_statMtxpTiles, g_statMclvChunks, g_statHoleChunks,
        g_statFailures, g_statWdlRead, g_statHeightTex, g_statDoodadModels, g_statMapObjects,
        g_statMapObjectsDropped, g_statLiquidLayers, g_statLiquidDegraded;

    SplitTile* FindTileLocked(void* area);   // caller holds g_mutex
    SplitTile* FindTileBrief(void* area);     // takes g_mutex only for the map access, then releases

    // ---------------------------------------------------------------- cross-unit responsibilities
    bool IsSplitTileName(const char* name, std::string& keyOut);  // AdtDetect
    void EnsureAlphaLayoutFlag();                                  // AdtDetect
    bool ParseAndPatchGuarded(SplitTile* t);                       // AdtParse
    bool InstallWdl();                                             // AdtWdl
    bool InstallTextures();                                        // AdtTexture
}
