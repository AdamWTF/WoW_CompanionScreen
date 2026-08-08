// Terrain tile/chunk lookups, the tile-slot grid, and runtime in-memory chunk field offsets.
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

#include <cstdint>
#include <cstddef>

// INTERNAL to the core. Terrain tile/chunk lookups, the tile-slot grid, and runtime in-memory chunk
// field offsets. Modules never include this; they use wxl::game / wxl::events.
namespace wxl::offsets::game::adt
{
    // --- lookups ---
    // Chunk lookup (pos) -> runtime chunk object, or null when that chunk is not parsed yet. A non-null
    // result means the chunk heightmap + collision are resident.
    constexpr uintptr_t kGetChunk = 0x007B49C0;
    // CMapChunk::Build (this=CMapChunk in ECX): turns one raw MCNK into a live chunk (sub-chunk pointers,
    // bbox, texture-layer units, ref spawn). The "a terrain chunk was built" point, distinct from the
    // per-frame terrain draw.
    constexpr uintptr_t kChunkBuild = 0x007C64B0;

    // Liquid-height query site that reads a LiquidType row's flag byte WITHOUT the null guard every other
    // liquid consumer has: an unknown liquid id (no row) faults here. The bytes `F6 40 08 04 74 08`
    // (test byte[eax+8],4 ; jz +8) are repatched so the test is skipped and the jump is unconditional,
    // taking the default (no water-surface bump) path like the guarded consumers.
    constexpr uintptr_t kLiquidRowFlagTest = 0x007C846C;

    // LiquidType.dbc id-index globals (filled at boot by the DBC load; live for the process).
    // row = rows[id - minId], and a gap id yields a NULL row -- presence must be tested before handing
    // an id to any stock liquid consumer. minId/maxId are the inclusive id bounds of the loaded table.
    constexpr uintptr_t kLiquidTypeDbRows  = 0x00AD4084; // LiquidTypeRec** (indexed by id - minId)
    constexpr uintptr_t kLiquidTypeDbMinId = 0x00AD4074; // u32 inclusive lower bound
    constexpr uintptr_t kLiquidTypeDbMaxId = 0x00AD4070; // u32 inclusive upper bound

    // LiquidTypeRec.MaterialID: Liquid::CMaterialBank::GetMaterial (0x008A1FA0) switches on this
    // column with no default case (1 water, 2 ooze, 3 magma) -- any other value caches a NULL
    // IMaterial* for the row, and the next liquid instance built from it null-derefs its material
    // pointer at draw time. The classic on-disk MCLQ path never reaches this with a bad id (it
    // hardcodes LiquidType id = layer-bit-index + 1, always 1..4, per CMapChunk::CreateLiquids
    // 0x007C5690); the native MH2O instance path (CMapChunk::CreateLiquids' second branch, fed by
    // FixupMh2o's normalized bytes) copies its `type` field straight through, so a row this column
    // doesn't cover is real: existence in the table is not the same as being usable.
    constexpr size_t kLiquidTypeMaterialId = 0x38; // u32, GetMaterial's usable range is {1, 2, 3}

    // TILE-AREA teardown (CMapArea::destructor, __thiscall via ECX=area) -- NOT a chunk destructor.
    // The historical name "ChunkDestroy" was a misnomer: this is the per-TILE object (CMapArea) whose
    // raw ADT file buffer at area+0x80 is freed here while a queued async-read completion may still
    // target it; a cancel hook retires the async object at area+0x70 before the free.
    constexpr uintptr_t kTileAreaDestroy = 0x007D6E10;
    using TileAreaDestroyFn = void(__fastcall*)(void* area);
    // Deprecated aliases (same address/field, kept so no published offset is ever deleted): the old
    // names wrongly said "chunk"; the object is the CMapArea tile. kOffChunkAsyncObj duplicates
    // kOffTileAsyncRead below -- it is the SAME +0x70 field of the SAME CMapArea object.
    constexpr uintptr_t kChunkDestroy = kTileAreaDestroy;    // deprecated: use kTileAreaDestroy
    using ChunkDestroyFn = TileAreaDestroyFn;                // deprecated: use TileAreaDestroyFn
    constexpr size_t kOffChunkAsyncObj = 0x70;               // deprecated: use kOffTileAsyncRead
    // Near-tile placed-object counter (chunk, &progress, total) -> count of placed-object children still
    // loading that overlap the chunk box.
    constexpr uintptr_t kNearObjectCount = 0x007B50B0;

    // --- tile-slot grid ---
    // Tile-slot grid base: a 64x64 array of tile-area pointers (stride 4). Slot index is
    // secondFilenameNumber * 64 + firstFilenameNumber, where the two numbers are the "%d_%d" of the
    // "<Map>_%d_%d.adt" tile name (area+0x48 = first, area+0x4C = second). NOTE the old comment said
    // "X-major (tileX*64 + tileY)": that was correct only under a swapped naming where "tileX" meant
    // the SECOND filename number. Phasing's PhaseHasTile uses the true second*64+first form.
    constexpr uintptr_t kTileSlots   = 0x00CE48D0;
    constexpr uint32_t  kTileGridDim = 64;   // tiles per axis
    constexpr size_t    kTileSlotStride = 0x04;
    // Detailed/streaming-path selector (u32).
    constexpr uintptr_t kStreamingPathSelector = 0x00CE0494;

    // --- tile-area (CMapArea) object fields ---
    constexpr size_t kOffTileAsyncRead  = 0x70; // CAsyncObject*; non-zero while a tile read is in flight
    constexpr size_t kOffTileFileBuffer = 0x80; // raw ADT byte buffer; freed by kTileAreaDestroy
    constexpr size_t kOffTileFileHandle = 0x6C; // SFile* of the open tile file (closed by async destroy)
    constexpr size_t kOffTileFileSize   = 0x84; // byte size of the +0x80 buffer
    constexpr size_t kOffTileIdxFirst   = 0x48; // first  %d of "<Map>_%d_%d.adt"
    constexpr size_t kOffTileIdxSecond  = 0x4C; // second %d of "<Map>_%d_%d.adt"

    // --- tile-area load / parse seam (used by the native split-ADT reader) ---
    // CMapArea::Load (__thiscall: ECX = area, one stack arg = tile filename): opens the tile file,
    // allocates the raw buffer (+0x80/+0x84) and queues the whole-file async read (+0x70) whose
    // main-thread completion is CMapArea::AsyncLoadCallback -> CMapArea::Create.
    constexpr uintptr_t kTileAreaLoad = 0x007D7150;
    using TileAreaLoadFn = void(__fastcall*)(void* area, void* edx, const char* filename);
    // CMapArea::Create (__thiscall via ECX, no args): the monolithic top-level parser. Reads ONLY
    // MVER + the 12 MHDR offsets of the buffer at area+0x80 and stores derived pointers/counts at
    // area+0x68..+0xB8 (MCIN/MTEX/MMDX/MMID/MWMO/MWID/MDDF/MODF/MFBO/MH2O/MTXF).
    constexpr uintptr_t kTileAreaCreate = 0x007D6EF0;
    using TileAreaCreateFn = void(__fastcall*)(void* area, void* edx);
    // Native async-read completion (__cdecl, ctx = area): Create + async destroy + zero +0x70/+0x6C.
    constexpr uintptr_t kTileAreaAsyncLoadCallback = 0x007D7020;
    // CMapArea::PrepareChunk (__thiscall: ECX = area, two stack args = grid row/col 0..15):
    constexpr uintptr_t kPrepareChunk = 0x007D6B30;
    using Map_PrepareChunkFn = void(__fastcall*)(void* area, void* edx, int row, int col);
    // CMapArea::Update (__thiscall: ECX = area, stack args = buildFlag, uint32_t bounds[4] = {colMin,
    // rowMin, colMax, rowMax})
    constexpr uintptr_t kAreaUpdate = 0x007D6BF0;
    using Map_AreaUpdateFn = void(__fastcall*)(void* area, void* edx, int buildFlag, uint32_t* bounds);
    // CMapChunk::ProcessIffChunks (__thiscall: ECX = chunk, one stack arg = firstBuild): the
    // SEQUENTIAL sub-chunk walk over the raw MCNK at chunk+0x10C that assigns the sub-chunk data
    // pointers at chunk+0x11C..+0x13C (3.3.5 never reads the MCNK-internal ofs* fields). Called only
    // by CMapChunk::Create. firstBuild!=0 patches MCNR/MCAL/MCLQ size fields in place once.
    constexpr uintptr_t kChunkProcessIffChunks = 0x007C3A10;
    using ChunkProcessIffChunksFn = void(__fastcall*)(void* chunk, void* edx, int firstBuild);
    // Raw tile-buffer allocator/free pair (plain SMemAlloc/SMemFree wrappers, MapMem.cpp). The free
    // takes (ptr, size) but ignores size. The tile destructor frees +0x80 through the free half.
    constexpr uintptr_t kAllocRawAreaData = 0x007BFE40;
    using AllocRawAreaDataFn = void*(__cdecl*)(uint32_t size);
    constexpr uintptr_t kFreeRawAreaData = 0x007BFE60;
    using FreeRawAreaDataFn = void(__cdecl*)(void* buffer, uint32_t size);
    // WDT MPHD flags global (first dword of the 0x20-byte MPHD copy): bit1 = MCCV vertex format,
    // bit2 = big (4096-byte, 8-bit) MCAL. Consulted live at every alpha unpack site.
    constexpr uintptr_t kMphdFlags = 0x00CF08D0;

    // --- map low-detail (WDL) seam ---
    // CMap::LoadWdl (MapLowDetail.cpp): opens "<mapPath>\<mapName>.wdl", SMemAlloc's the whole file
    // into wdlState[0], then parses MVER -> optional MWMO/MWID/MODF -> MAOF -> per-tile MARE(+MAHO).
    // Convention BYTE-VERIFIED against the 3.3.5.12340 export: true __thiscall (prologue
    // 55 8B EC 81 EC 3C 01 00 00 .. 8B F9 = this out of ECX, epilogue C2 08 00 = two stack args),
    // returns 1 on success / 0 when the .wdl does not open. Single caller: CMap__Load @ 0x007BFDD2
    // with ECX = kWdlState and args (&CMap__mapPath, &CMap__mapName). Declared __fastcall with a
    // dummy EDX so the trampoline routes wdlState into the this-register.
    constexpr uintptr_t kLoadWdl = 0x007CC310;
    using LoadWdlFn = uint32_t(__fastcall*)(int* wdlState, void* edx,
                                            const char* mapPath, const char* mapName);
    // The CMap WDL state block (the ECX of kLoadWdl), an int[0x100A] global:
    //   [0]           raw .wdl file buffer (SMemAlloc; the unload SMemFree's it)
    //   [1..3]        MWMO data / MWID data / MODF data pointers (WMO-only maps; else stale-zero)
    //   [4]           MODF entry count (MODF size >> 6)
    //   [5]           MAOF offset table = 64*64 u32 file offsets, 0 = no low-detail tile
    //   [6..0x1005]   the 64x64 CMapAreaLow* tile-slot array (kWdlSlotCount entries)
    //   [0x1006..0x1009] the low-detail map-obj-def growable-array block
    // Zeroed whole at startup by the static ctor 0x007CC2C0, and on every map unload by
    // CMap::UnloadWdl 0x007CC770 (frees+zeroes every slot, zeroes [1..5]/[0x1007], SMemFree's [0]).
    // CMap__Load runs CMap__Purge (-> 0x007CC770 @ 0x007C3843) BEFORE kLoadWdl, so the block is
    // always clean when kLoadWdl is entered.
    constexpr uintptr_t kWdlState     = 0x00CF0900;
    constexpr uint32_t  kWdlSlotCount = 64 * 64; // dimension of the [6..] slot array (0x1000)
    // CMap::AllocAreaLow: pool-allocates one CMapAreaLow (the per-tile low-detail object stored in
    // the kWdlState [6..] slots). BYTE-VERIFIED __cdecl, no args, pointer in EAX (prologue
    // 55 8B EC 83 EC 08 8B 15 18 54 D2 00 -- pool head at 0xD25418 -- plain C3 ret). Fields below are
    // what the native kLoadWdl grid loop writes on the returned object; see AreaLow for the typed view.
    constexpr size_t kOffAreaLowMinX         = 0x04;
    constexpr size_t kOffAreaLowMinY         = 0x08;
    constexpr size_t kOffAreaLowMinZ         = 0x0C;
    constexpr size_t kOffAreaLowMaxX         = 0x10;
    constexpr size_t kOffAreaLowMaxY         = 0x14;
    constexpr size_t kOffAreaLowMaxZ         = 0x18;
    constexpr size_t kOffAreaLowCenterX      = 0x1C;
    constexpr size_t kOffAreaLowCenterY      = 0x20;
    constexpr size_t kOffAreaLowCenterZ      = 0x24;
    constexpr size_t kOffAreaLowRadius       = 0x28;
    constexpr size_t kOffAreaLowOriginX      = 0x2C;
    constexpr size_t kOffAreaLowOriginY      = 0x30;
    constexpr size_t kOffAreaLowCol          = 0x38;
    constexpr size_t kOffAreaLowRow          = 0x3C;
    constexpr size_t kOffAreaLowRenderBudget = 0x40; // byte budget for render indices, 0xC per unholed cell
    constexpr size_t kOffAreaLowMareData     = 0x44; // -> 545 s16 heightmap (17x17 outer + 16x16 inner)
    constexpr size_t kOffAreaLowMahoData     = 0x48; // -> 16 u16 hole mask, or 0
    constexpr uintptr_t kAllocAreaLow = 0x007C0A90;
    using AllocAreaLowFn = void*(__cdecl*)();
    // CMap::FreeAreaLow (landmark, not called by the core): the unload 0x007CC770 releases every
    // non-null kWdlState slot through it before zeroing the slot.
    constexpr uintptr_t kFreeAreaLow = 0x007C0C60;

    // --- runtime chunk object fields ---
    constexpr size_t kOffChunkNodeLayerCount = 0x09; // draw-node (CMapRenderChunk) layer count
    // CMapChunk -> MCNK 128-byte data header (= raw MCNK ptr + 8-byte tag). The authoritative texture-layer
    // count (SMChunk.nLayers, 0..4) lives at header + 0x0C.
    constexpr size_t kOffChunkMcnkHeader = 0x110;
    constexpr size_t kOffMcnkNLayers     = 0x0C;
    // Raw on-disk MCLY/MCAL base pointers (point into the resident MCNK block, all physical entries, not
    // just the 4 materialized layers). The 4-byte field right before the MCLY payload is its sub-chunk
    // size, so physical-layer-count = *(mclyBase - 4) / 0x10.
    constexpr size_t kOffChunkMcly       = 0x12C;
    constexpr size_t kOffChunkMcal       = 0x130;
    // The full CMapChunk sub-chunk pointer block ProcessIffChunks fills (+0x11C..+0x13C). Every
    // consumer (vertex/bounds/intersect/alpha/shadow/liquid/refs/sound builds) reads these LIVE, so
    // whatever they point at must stay resident for the whole tile lifetime.
    constexpr size_t kOffChunkRawMcnk    = 0x10C; // raw MCNK (tag+size header) inside the tile buffer
    constexpr size_t kOffChunkMcvt       = 0x11C; // 145 floats, relative heights
    constexpr size_t kOffChunkMccv       = 0x120; // 145 x BGRA vertex colors (vertex format 2 only)
    constexpr size_t kOffChunkMcnr       = 0x124; // 435 signed normal bytes
    constexpr size_t kOffChunkMcsh       = 0x128; // 512-byte shadow bitmap (hdr flags bit0 gates use)
    constexpr size_t kOffChunkMcrf       = 0x134; // u32 refs: doodads first (nDoodadRefs) then wmos
    constexpr size_t kOffChunkMclq       = 0x138; // legacy liquid layers (hdr sizeLiquid > 8 gates)
    constexpr size_t kOffChunkMcse       = 0x13C; // sound emitters (hdr nSndEmitters gates)
    // Primitive/draw-batch descriptor (the 145-vertex MCVT grid VB/IB) passed to the device Draw method.
    constexpr size_t kOffChunkDrawBatch  = 0x90;
    // Source of the tile tex-owner object: (*(chunkObj+0x20) & ~1) + 8.
    constexpr size_t kOffChunkTexOwnerSrc = 0x20;
    // Per-layer record array (4 slots, stride 0x14): +0x00 flags, +0x04 diffuse CGxTex*, +0x0C alpha
    // CGxTex*, +0x10 back-ptr. Only the first nLayers (<=4) records exist.
    constexpr size_t kOffChunkLayerRecords   = 0x34;
    constexpr size_t kChunkLayerRecordStride = 0x14;

    // --- terrain surface render (per-chunk draw + multi-pass extension) ---
    // Per-chunk surface draw leaf (chunkObj is a stack arg, __cdecl): binds {layer diffuse @ sampler
    // 0x15, layer alpha @ sampler 0x16} and issues one DrawIndexedPrimitive per layer (layer 0 opaque,
    // 1..n alpha-over). The active variant is held in kSurfaceDrawFnPtr; this is the default body.
    constexpr uintptr_t kSurfaceChunkDraw  = 0x007D0D70;
    // Per-chunk draw fn-ptr the surface render dispatches through (selected by the draw-variant selector).
    constexpr uintptr_t kSurfaceDrawFnPtr  = 0x00D25098;
    // Every per-chunk surface-draw body the selector may install into kSurfaceDrawFnPtr: default, shadow,
    // and the shader/hi-detail bodies, picked by the active graphics config. These are called through the
    // indirect dispatch (__cdecl, chunkObj on the stack).
    constexpr uintptr_t kSurfaceChunkDrawVariants[] = {
        0x007D0D70, 0x007D0760, 0x007D13F0, 0x007D1AD0, 0x007D20A0, 0x007D2520,
    };
    // The shader-path per-chunk surface draw, called DIRECTLY by the surface driver (not via the indirect
    // dispatch) when the pixel-shader terrain path is active. Convention is __thiscall (chunkObj in ECX).
    // It draws one chunk per call with a single DIP: diffuse layer i at stage 0x15+i, a 4-channel combined
    // alpha RT (chunkObj+0x84) at stage 0x15+nLayers, and a Terrain1/2/3 pixel shader indexed by nLayers.
    constexpr uintptr_t kSurfaceChunkDrawShader = 0x007D2D70;
    // CGxDevice singleton; vtable + 0xA8 = the Draw (DrawIndexedPrimitive) method (batch ptr + flag).
    constexpr uintptr_t kGxDeviceSingleton = 0x00C5DF88;
    constexpr size_t    kGxDeviceDrawVtbl  = 0xA8;
    // CGxTex -> GxTex GPU handle resolve.
    constexpr uintptr_t kTexResolve        = 0x004B6CB0;
    // GxRsSet / SetTexture for a sampler slot (0x15 = diffuse stage, 0x16 = alpha stage).
    constexpr uintptr_t kSetSamplerTexture = 0x00685F50;
    // Sampler addr/filter state for the just-bound texture.
    constexpr uintptr_t kSetSamplerState   = 0x00681450;
    // Lazy texture loader for one tex-owner handle slot: slot[+4] = Load(slot[+0]).
    constexpr uintptr_t kLazyLoadTexSlot   = 0x007D6980;
    // CMapArea::LoadTextures: builds the tile tex-owner handle array (area+0x60) from the MTEX name
    // blob -- one {name, CGxTex*} slot per NUL-terminated name, indexed by MCLY.textureId, eager-
    // loading each through kLazyLoadTexSlot unless SFile streaming mode defers it. Native this-in-ECX
    // (the CMapArea) + (mtexData, mtexSize) on the stack; declared __fastcall with a dummy EDX.
    // Detoured for split tiles to source the names from the real MDID (FileDataID) instead of MTEX.
    constexpr uintptr_t kAreaLoadTextures  = 0x007D6D20;
    using Map_AreaLoadTexturesFn = void(__fastcall*)(void* area, void* edx, const void* mtexData, uint32_t mtexSize);
    // kLazyLoadTexSlot signature: __thiscall(area, slot, index); slot = { char* name; CGxTex* tex }.
    using Map_LoadTerrainTextureFn = void(__fastcall*)(void* area, void* edx, void** slot, uint32_t index);
    // Builds the per-layer alpha texture from a layer record's MCAL into record + 0x0C.
    constexpr uintptr_t kBuildLayerAlpha   = 0x007B9DE0;
    constexpr uint32_t  kSamplerDiffuse    = 0x15;
    constexpr uint32_t  kSamplerAlpha      = 0x16;
    // Tile tex-owner: per-tile texture-handle array, indexed by MCLY.textureId, stride 8
    // ([+0] = MTEX filename ptr, [+4] = loaded CGxTex*). Covers the whole tile MTEX set.
    constexpr size_t kOffTexOwnerHandleArray = 0x60;
    constexpr size_t kTexOwnerHandleStride   = 0x08;

    // --- terrain height blend ---
    // Shader-path per-chunk draw signature: native this-in-ECX, no stack args. Declared __fastcall
    // with a dummy EDX so the trampoline routes the chunk into the this-register.
    using Map_SurfaceChunkDrawShaderFn = void(__fastcall*)(void* chunkObj, void* edx);
    // ACTIVE terrain pixel-shader table: CGxShader*[4], one slot per layer count 1..4. Rewritten once
    // per bucket per frame by CMapRenderChunk::SetShaders (0x007D3E10) and bound at GxRs 0x4E by the
    // bucket loop BEFORE the per-chunk draw leaf runs -- so at kSurfaceChunkDrawShader time,
    // slot[nLayers-1] is the stock wrapper the pending DIP will use. A detour that swaps GxRs 0x4E to
    // its own wrapper before the original leaf (and restores after) owns that one chunk's PS.
    constexpr uintptr_t kActiveTerrainPs = 0x00D1D080;
    // CMap::enableTerrainShaderPixel byte gate: non-zero when the pixel-shader terrain path is
    // active (the only path kSurfaceChunkDrawShader runs under). Same RE doc, section 1.2.
    constexpr uintptr_t kEnableTerrainShaderPixel = 0x00CE049E;
    // GxRs state indices for the deferred shader binds (same setter as kSetSamplerTexture): the
    // bucket loop writes the terrain PS wrapper at 0x4E; the pre-draw GxState flush applies the
    // wrapper's live handle (+0x20, created flag +0x30) to the device. Mirrors
    // offsets/engine/Shader.hpp kStateVertexShader/kStatePixelShader.
    constexpr uint32_t kGxStateVertexShader = 0x4D;
    constexpr uint32_t kGxStatePixelShader  = 0x4E;
    // By-name map texture loader: builds the wrap/filter flag set and creates the texture handle.
    // Returns the texture handle, 0 on failure. Content streams in asynchronously.
    constexpr uintptr_t kMapLoadTexture = 0x007D9990;
    using Map_LoadTextureFn = void*(__cdecl*)(const char* filename);
    // First free sampler selector on the terrain draw (s9 = selector 0x1E); s9..s15 stay engine-free
    // there (selectors 0x1E..0x24 map linearly to s9..s15). The first pass binds its four height
    // textures at s9..s12; the extra-layer second pass splits the same range: extras' heights at
    // s9..s11, the natives' combined alpha at s12, and the natives' heights at s13..s15.
    constexpr uint32_t kSamplerSelHeight0     = 0x1E;
    constexpr uint32_t kSamplerSelNativeAlpha = 0x21; // s12 in the second pass
    constexpr uint32_t kSamplerSelNativeH0    = 0x22; // s13..s15 in the second pass
    constexpr uint32_t kSamplerSelFreeCount   = 7;    // 0x1E..0x24 = s9..s15
    // First engine-free terrain pixel-shader constant: c13..c16 = per-layer (heightScale, heightOffset)
    // in the first pass. The second pass extends the range: c13..c15 = extras' pairs, c16..c19 = the
    // natives' pairs, c20 = the natives' UV-tiling ratios relative to the pass draw's layer 0.
    constexpr uint32_t kPsConstHeightBase      = 13;
    constexpr uint32_t kPsConstNativeHeight    = 16;
    constexpr uint32_t kPsConstNativeUvRatio   = 20;
    constexpr uint32_t kPsConstSecondPassCount = 8; // c13..c20 uploaded as one block
    // Served-terrain-shader contract: c21 = the extras' UV-tiling ratios; c13..c21 as one block.
    constexpr uint32_t kPsConstExtrasUvRatio   = 21;
    constexpr uint32_t kPsConstTerrainBindCount = 9;
    // Relocated served-terrain-shader block: c22..c24 extras pairs, c25..c27 native pairs, c28.y
    // native layer-3 height, c29 native uv ratios, c30 extras uv ratios (c13..c21 collided with
    // the additive shader family's own constant use; c22..c30 verified free on every permutation).
    constexpr uint32_t kPsConstTerrainBindBase = 22;
    // Signatures for kTexResolve / kSetSamplerTexture above.
    using Map_TexResolveFn  = void*(__cdecl*)(void* handle, int a, int b);
    using Map_SamplerBindFn = void(__fastcall*)(void* device, void* edx, uint32_t selector, void* tex);

    // --- terrain extra-layer second pass (layers 5..8) ---
    // Draw-node fields the second blended draw mutates and restores around the redraw. The draw leaf
    // (kSurfaceChunkDrawShader) never writes the node, so a mutate/redraw/restore inside a detour on
    // it is safe; the leaf re-runs the VS pick and full constant upload each call.
    constexpr size_t kOffChunkNodeFlags   = 0x0A; // u16: bit0 = mask-family layer, bit2 = cube-env layer
    constexpr size_t kOffChunkNodeChunk   = 0x10; // CMapChunk* backing the node
    constexpr size_t kOffChunkNodeAlphaRT = 0x84; // combined alpha texture handle bound at s(nLayers)
    // Layer record fields (record = node + kOffChunkLayerRecords + i*kChunkLayerRecordStride).
    constexpr size_t kOffLayerSlotFlags = 0x00;   // u16 MCLY flags low word (anim dir/speed/animate)
    constexpr size_t kOffLayerSlotIndex = 0x02;   // u16 slot index (0..3)
    constexpr size_t kOffLayerSlotTexId = 0x08;   // u32 MCLY textureId (dedup key only at draw time)
    constexpr size_t kOffLayerSlotAlpha = 0x0C;   // per-layer alpha handle; 0 on the shader path
    constexpr size_t kOffLayerSlotNode  = 0x10;   // back-pointer to the node
    // CMapChunk identity fields (engine-written, not data-trusted).
    constexpr size_t kOffMapChunkIndexX  = 0x24;  // local 0..15 (MCIN slot = y*16 + x)
    constexpr size_t kOffMapChunkIndexY  = 0x28;
    constexpr size_t kOffMapChunkGlobalX = 0x34;  // tileX*16 + localX (0..1023)
    constexpr size_t kOffMapChunkGlobalY = 0x38;
    // In-memory terrain texture create: builds linear/clamp flags and creates a callback-filled
    // texture handle. The fill callback is invoked by the texture system with op==1 and must write
    // *outBase / *outStride; the creation ctx arrives as its 6th argument (stride at arg 7, base at
    // arg 8, byte-verified against the native terrain fill callback).
    constexpr uintptr_t kAllocTerrainTexture = 0x007B7A70;
    using Map_AllocTerrainTextureFn = void*(__cdecl*)(uint32_t w, uint32_t h, void* ctx, void* callback,
                                                      uint32_t fmt, uint32_t fmt2);
    using Map_TexFillCallbackFn = void(__cdecl*)(int op, uint32_t w, uint32_t h, uint32_t a4,
                                                 uint32_t a5, void* ctx, uint32_t* outStride,
                                                 const void** outBase);
    constexpr uint32_t kTexFormatArgb8888 = 2;
    // Texture handle release (pairs with kAllocTerrainTexture / kMapLoadTexture).
    constexpr uintptr_t kTextureRelease = 0x0047BF30;
    using TextureReleaseFn = void(__cdecl*)(void* handle);
    // Global render-state setter (id, value): master-gated, writes the state cell + marks it dirty;
    // the next draw's state sync flushes it to the device.
    constexpr uintptr_t kGxRsSetInt = 0x00408BF0;
    using GxRsSetIntFn = void(__cdecl*)(int id, int value);
    constexpr int kGxRsBlend    = 6; // blend mode enum; 0 = opaque, 2 = srcAlpha / invSrcAlpha
    constexpr int kGxRsAlphaRef = 7; // alpha-test ref; 0 = off, 1 = discard zero-coverage pixels
    // Shadow tier getter (0 = no shadow). Tier 0 pairs the terrain PS with the unpacked-texcoord VS
    // family; tiers 1..3 pair with the packed family (layer 2/3 uvs share one texcoord register).
    constexpr uintptr_t kShadowTierGetter = 0x00873FF0;
    using ShadowTierGetterFn = int(__cdecl*)();

    // --- terrain per-layer UV scale ---
    // Builds the per-chunk terrain shader constant block (the 37 vec4s) and uploads it. c18..c21 are the
    // four per-layer UV-tiling vec4s. Post-hooked to divide each drawn layer's c18+i.xy by its texture's
    // modern scale (1<<exponent) and re-upload c18..c(18+layerCount-1). __cdecl, node = first arg; the node
    // is the CMapChunk (layer count at kOffChunkNodeLayerCount, layer slots at kOffChunkLayerRecords).
    constexpr uintptr_t kBuildTerrainConstants = 0x007D0050;
    using Map_BuildTerrainConstantsFn = void(__cdecl*)(void* node, uint32_t a1, uint32_t a2);
    // c18 constant data in memory (4 floats per register); c18 is the first per-layer tiling vec4.
    constexpr uintptr_t kVsConstC18    = 0x00D251C0;
    constexpr uint32_t  kVsConstC18Reg = 18; // start register for the re-upload
    // Resolved-texture pointer inside a layer slot (slot = node + kOffChunkLayerRecords + i*kChunkLayerRecordStride).
    constexpr size_t kOffLayerSlotTexture = 0x04;
    // File-name string (NUL-terminated) inside a resolved texture object.
    constexpr size_t kOffTextureName = 0x6C;

    // --- signatures ---
    // Chunk lookup (pos on stack) -> chunk object.
    using Map_GetChunkFn = void*(__cdecl*)(float* pos);
    // Near-object counter (chunk, progressOut, total) -> count.
    using Map_NearObjectCountFn = int(__cdecl*)(void* chunk, int* progressOut, int total);
    // CMapChunk::Build: native this-in-ECX (__thiscall, ret 8). Declared __fastcall with a dummy EDX so the
    // trampoline routes the chunk into the this-register and keeps the two stack args.
    using Map_ChunkBuildFn = void(__fastcall*)(void* chunk, void* edx, void* rawMcnk, int param2);

    // --- typed views over the objects above ---
    // The constants are the curated landmarks; these structs give named, typed access to the same fields,
    // with every member offset checked against a constant at compile time (a wrong padding fails the build).
    // Only RE'd fields are named; the gaps are explicit padding. Pointers are 4 bytes on the 32-bit client.
#pragma pack(push, 1)
    /**
     * @brief Tile-area object (one per resident map tile): filename index, file handle, async-read
     *        state, file buffer.
     *
     * Pointer-valued fields are stored as uint32_t, not void*: with more than one such field in the same
     * struct, sizeof(void*) would drive the padding between them, and this header is 32/64-bit-neutral
     * (sizeof(uint32_t) is not). Only ever the LAST field of a struct is safe to type as a real pointer.
     */
    struct TileArea
    {
        uint8_t  _pad00[kOffTileIdxFirst];
        int32_t  tileFirst;        // kOffTileIdxFirst  (first  %d of "<Map>_%d_%d.adt")
        int32_t  tileSecond;       // kOffTileIdxSecond (second %d of "<Map>_%d_%d.adt")
        uint8_t  _pad50[kOffTileFileHandle - (kOffTileIdxSecond + sizeof(int32_t))];
        uint32_t fileHandle;       // kOffTileFileHandle (SFile* of the open tile file)
        uint32_t asyncRead;        // kOffTileAsyncRead  (non-zero while the root read is in flight)
        uint8_t  _pad74[kOffTileFileBuffer - (kOffTileAsyncRead + sizeof(uint32_t))];
        uint32_t fileBuffer;       // kOffTileFileBuffer (non-zero once the file buffer is allocated)
        uint32_t fileSize;         // kOffTileFileSize   (byte size of the +0x80 buffer)
    };
    static_assert(offsetof(TileArea, tileFirst)  == kOffTileIdxFirst,  "TileArea.tileFirst");
    static_assert(offsetof(TileArea, tileSecond) == kOffTileIdxSecond, "TileArea.tileSecond");
    static_assert(offsetof(TileArea, fileHandle) == kOffTileFileHandle, "TileArea.fileHandle");
    static_assert(offsetof(TileArea, asyncRead)  == kOffTileAsyncRead,  "TileArea.asyncRead");
    static_assert(offsetof(TileArea, fileBuffer) == kOffTileFileBuffer, "TileArea.fileBuffer");
    static_assert(offsetof(TileArea, fileSize)   == kOffTileFileSize,   "TileArea.fileSize");

    /**
     * @brief Runtime chunk object (CMapChunk): tex-owner link, local grid index, and the sub-chunk
     *        pointer block ProcessIffChunks fills (raw MCNK, header, and each parsed sub-chunk).
     *
     * The old single struct conflated two objects: nodeLayerCount @0x09 is a CMapRenderChunk (draw
     * node) field, while everything here is a CMapChunk field. They are now two typed views --
     * MapChunk for the CMapChunk, RenderNode for the CMapRenderChunk reached via chunk+0xA8.
     */
    struct MapChunk
    {
        uint8_t  _pad00[kOffChunkTexOwnerSrc];
        uint32_t texOwnerSrc;      // kOffChunkTexOwnerSrc (tagged link; tile tex-owner = (v & ~1) + 8)
        int32_t  indexX;           // kOffMapChunkIndexX (local 0..15, MCIN slot = y*16 + x)
        int32_t  indexY;           // kOffMapChunkIndexY
        uint8_t  _pad2C[kOffChunkRawMcnk - (kOffMapChunkIndexY + sizeof(int32_t))];
        uint32_t rawMcnk;          // kOffChunkRawMcnk    (raw MCNK tag+size header in the tile buffer)
        uint32_t mcnkHeader;       // kOffChunkMcnkHeader -> McnkHeader (raw MCNK ptr + 8-byte tag)
        uint8_t  _pad114[kOffChunkMcvt - (kOffChunkMcnkHeader + sizeof(uint32_t))];
        uint32_t mcvt;             // kOffChunkMcvt (145 floats, relative heights)
        uint32_t mccv;             // kOffChunkMccv (145 x BGRA vertex colors, vertex format 2 only)
        uint32_t mcnr;             // kOffChunkMcnr (435 signed normal bytes)
        uint32_t mcsh;             // kOffChunkMcsh (512-byte shadow bitmap, header flags bit0 gates use)
        uint32_t mcly;             // kOffChunkMcly (raw on-disk layer records)
        uint32_t mcal;             // kOffChunkMcal (raw on-disk alpha maps)
        uint32_t mcrf;             // kOffChunkMcrf (u32 refs: doodads first, then wmos)
        uint32_t mclq;             // kOffChunkMclq (legacy liquid layers, header sizeLiquid > 8 gates)
        uint32_t mcse;             // kOffChunkMcse (sound emitters, header nSndEmitters gates)
    };
    static_assert(offsetof(MapChunk, texOwnerSrc) == kOffChunkTexOwnerSrc,  "MapChunk.texOwnerSrc");
    static_assert(offsetof(MapChunk, indexX)      == kOffMapChunkIndexX,   "MapChunk.indexX");
    static_assert(offsetof(MapChunk, indexY)      == kOffMapChunkIndexY,   "MapChunk.indexY");
    static_assert(offsetof(MapChunk, rawMcnk)     == kOffChunkRawMcnk,    "MapChunk.rawMcnk");
    static_assert(offsetof(MapChunk, mcnkHeader)  == kOffChunkMcnkHeader, "MapChunk.mcnkHeader");
    static_assert(offsetof(MapChunk, mcvt)        == kOffChunkMcvt,       "MapChunk.mcvt");
    static_assert(offsetof(MapChunk, mccv)        == kOffChunkMccv,       "MapChunk.mccv");
    static_assert(offsetof(MapChunk, mcnr)        == kOffChunkMcnr,       "MapChunk.mcnr");
    static_assert(offsetof(MapChunk, mcsh)        == kOffChunkMcsh,       "MapChunk.mcsh");
    static_assert(offsetof(MapChunk, mcly)        == kOffChunkMcly,       "MapChunk.mcly");
    static_assert(offsetof(MapChunk, mcal)        == kOffChunkMcal,       "MapChunk.mcal");
    static_assert(offsetof(MapChunk, mcrf)        == kOffChunkMcrf,       "MapChunk.mcrf");
    static_assert(offsetof(MapChunk, mclq)        == kOffChunkMclq,       "MapChunk.mclq");
    static_assert(offsetof(MapChunk, mcse)        == kOffChunkMcse,       "MapChunk.mcse");

    /**
     * @brief One MCLY layer slot inside a draw node's layer array
     *        (record = node + kOffChunkLayerRecords + i*kChunkLayerRecordStride).
     */
    struct LayerRecord
    {
        uint8_t  _pad00[kOffLayerSlotTexId];
        uint32_t texId;    // kOffLayerSlotTexId (MCLY textureId, dedup key only at draw time)
        uint8_t  _pad0C[kChunkLayerRecordStride - (kOffLayerSlotTexId + sizeof(uint32_t))];
    };
    static_assert(offsetof(LayerRecord, texId) == kOffLayerSlotTexId,   "LayerRecord.texId");
    static_assert(sizeof(LayerRecord)          == kChunkLayerRecordStride, "LayerRecord size/stride");

    /** @brief Draw node (CMapRenderChunk, chunk+0xA8): layer count/flags, owning chunk, layer array. */
    struct RenderNode
    {
        uint8_t     _pad00[kOffChunkNodeLayerCount];
        uint8_t     nodeLayerCount;   // kOffChunkNodeLayerCount (draw-node layer count)
        uint16_t    nodeFlags;        // kOffChunkNodeFlags (bit0 = mask-family layer, bit2 = cube-env layer)
        uint8_t     _pad0C[kOffChunkNodeChunk - (kOffChunkNodeFlags + sizeof(uint16_t))];
        uint32_t    nodeChunk;        // kOffChunkNodeChunk -> MapChunk backing the node
        uint8_t     _pad14[kOffChunkLayerRecords - (kOffChunkNodeChunk + sizeof(uint32_t))];
        LayerRecord layers[4];        // kOffChunkLayerRecords; only [0..nodeLayerCount) are valid
    };
    static_assert(offsetof(RenderNode, nodeLayerCount) == kOffChunkNodeLayerCount, "RenderNode.nodeLayerCount");
    static_assert(offsetof(RenderNode, nodeFlags)      == kOffChunkNodeFlags,      "RenderNode.nodeFlags");
    static_assert(offsetof(RenderNode, nodeChunk)      == kOffChunkNodeChunk,      "RenderNode.nodeChunk");
    static_assert(offsetof(RenderNode, layers)         == kOffChunkLayerRecords,   "RenderNode.layers");
    static_assert(offsetof(RenderNode, layers) + sizeof(RenderNode::layers) == kOffChunkNodeAlphaRT,
                  "RenderNode.layers should end exactly at the combined alpha-RT slot");

    /** @brief MCNK 128-byte data header (chunk->mcnkHeader): the authoritative texture-layer count. */
    struct McnkHeader
    {
        uint8_t  _pad00[kOffMcnkNLayers];
        uint32_t nLayers;          // kOffMcnkNLayers (SMChunk.nLayers, 0..4)
    };
    static_assert(offsetof(McnkHeader, nLayers) == kOffMcnkNLayers, "McnkHeader.nLayers");

    /**
     * @brief Low-detail tile object (CMapAreaLow, from CMap::AllocAreaLow): the WDL grid-loop bounds,
     *        column/row, render-index budget, and MARE/MAHO data.
     */
    struct AreaLow
    {
        uint8_t  _pad00[kOffAreaLowMinX];
        float    minX;
        float    minY;
        float    minZ;
        float    maxX;
        float    maxY;
        float    maxZ;
        float    centerX;
        float    centerY;
        float    centerZ;
        float    radius;
        float    originX;
        float    originY;
        uint8_t  _pad34[kOffAreaLowCol - (kOffAreaLowOriginY + sizeof(float))];
        int32_t  col;               // kOffAreaLowCol
        int32_t  row;               // kOffAreaLowRow
        uint32_t renderBudget;      // kOffAreaLowRenderBudget
        uint32_t mareData;          // kOffAreaLowMareData (raw address; see MapChunk's note on why)
        uint32_t mahoData;          // kOffAreaLowMahoData (raw address, or 0)
    };
    static_assert(offsetof(AreaLow, minX)         == kOffAreaLowMinX,         "AreaLow.minX");
    static_assert(offsetof(AreaLow, minY)         == kOffAreaLowMinY,         "AreaLow.minY");
    static_assert(offsetof(AreaLow, minZ)         == kOffAreaLowMinZ,         "AreaLow.minZ");
    static_assert(offsetof(AreaLow, maxX)         == kOffAreaLowMaxX,         "AreaLow.maxX");
    static_assert(offsetof(AreaLow, maxY)         == kOffAreaLowMaxY,         "AreaLow.maxY");
    static_assert(offsetof(AreaLow, maxZ)         == kOffAreaLowMaxZ,         "AreaLow.maxZ");
    static_assert(offsetof(AreaLow, centerX)      == kOffAreaLowCenterX,      "AreaLow.centerX");
    static_assert(offsetof(AreaLow, centerY)      == kOffAreaLowCenterY,      "AreaLow.centerY");
    static_assert(offsetof(AreaLow, centerZ)      == kOffAreaLowCenterZ,      "AreaLow.centerZ");
    static_assert(offsetof(AreaLow, radius)       == kOffAreaLowRadius,       "AreaLow.radius");
    static_assert(offsetof(AreaLow, originX)      == kOffAreaLowOriginX,      "AreaLow.originX");
    static_assert(offsetof(AreaLow, originY)      == kOffAreaLowOriginY,      "AreaLow.originY");
    static_assert(offsetof(AreaLow, col)          == kOffAreaLowCol,          "AreaLow.col");
    static_assert(offsetof(AreaLow, row)          == kOffAreaLowRow,          "AreaLow.row");
    static_assert(offsetof(AreaLow, renderBudget) == kOffAreaLowRenderBudget, "AreaLow.renderBudget");
    static_assert(offsetof(AreaLow, mareData)     == kOffAreaLowMareData,     "AreaLow.mareData");
    static_assert(offsetof(AreaLow, mahoData)     == kOffAreaLowMahoData,     "AreaLow.mahoData");
#pragma pack(pop)
}
