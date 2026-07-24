// Model load / animation / batch-alpha / ribbon entry addresses, signatures, and object field offsets.
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

// INTERNAL to the core. Client addresses and runtime object field offsets. This is the private
// SOURCE the game-binding catalog is curated from; modules never include it, they call wxl::game.
namespace wxl::offsets::game::m2
{
    // --- load / setup ---
    // Model init: parses a model file and builds the runtime model.
    constexpr uintptr_t kInit = 0x0083CF00;
    // Skin-profile finalizer: runs once after the skin sub-arrays resolve and before the shader passes
    // size their batch blocks. The point to rebuild the material contract a modern skin omits.
    // CAUTION: calls kBuildBatchMaterial per batch. kBuildBatchMaterial crashes (EBX=0 null-deref at
    // 0x836D11) when M2Batch::shaderId has bit 0x8000 set AND (shaderId & 0x7FFF) > 3 — a missing
    // switch case patched by our hkBuildBatchMaterial hook. Calling kFinalizeSkin a second time
    // also leaks model+0x18C (submesh copy), model+0x188 (per-submesh objects), [model+0x150]+0x40
    // (IB staging buffer) — no free-before-write; acceptable for infrequent equip cycles.
    constexpr uintptr_t kFinalizeSkin = 0x00837A40;
    // Per-batch material-key builder: thiscall (ECX=model), 1 stack arg (batch ptr from skin->batches).
    // Called by kFinalizeSkin once per skin batch; result stored in model+0x188 array.
    // Reads M2Batch::shaderId (batch+2): bit 0x8000 selects the path; bits 0-14 index the switch.
    // Switch handles values 0-3 only; values > 3 with bit 0x8000 are an unimplemented case that
    // crashes. Hook (hkBuildBatchMaterial in GameHooks.cpp) returns nullptr to skip those safely.
    constexpr uintptr_t kBuildBatchMaterial = 0x00836C90;
    using M2_BuildBatchMaterialFn = void* (__fastcall*)(void* model, void* edx, void* batchPtr);

    // Version-gate branches in the loader. The stock loader accepts only one inner version; these are
    // the two compare branches that reject higher inner versions.
    constexpr uintptr_t kVersionGateInit = 0x0083CF51; // version-too-high branch
    constexpr uintptr_t kVersionGateAnim = 0x0083C745; // anim-parse version branch

    // --- native modern-M2 direct-fill entry points (features/m2native) ---
    // CM2Shared::Initialize (0x83CC80): the half of kInit that runs AFTER the header offset->pointer
    // walk. Chooses the skin profile from the device bone budget, calls kLoadSkinProfile (name-based
    // "%s%02d.skin"), allocates the texture-handle array at model+0x174 and creates one handle per
    // M2Texture (TextureCreate on the filename pointer; solid white when filename.count < 2), flags
    // materials with blend > 4, and counts billboarded bones into model+0x198. thiscall, no args,
    // called exactly once from kInit. Verified: prologue 55 8B EC 83 EC 08 53 56 8B F1 and the
    // "f640 04 08" caps test at +0xD match the decompiled CM2Shared__Initialize (02_functions.csv
    // row 0083cc80; byte-checked against Wow.exe .text).
    constexpr uintptr_t kSharedInitialize = 0x0083CC80;
    using M2_SharedInitializeFn = int(__fastcall*)(void* model);

    // Storm SMemAlloc (0x76E540, __stdcall(size, name, line, flags), ret 0x10 verified) -- the
    // allocator kInit uses for the external-sequence bookkeeping array it stores at model+0x28
    // (count at +0x24, cursor zeroed at +0x2C). A native reader replicating kInit's tail MUST use
    // this exact allocator so the model destructor's matching SMemFree stays valid. Call-site
    // constants replicated from kInit: name ".\\M2Shared.cpp", line 0x2DC, flags 8.
    constexpr uintptr_t kSMemAlloc = 0x0076E540;
    using SMemAllocFn = void*(__stdcall*)(uint32_t size, const char* name, uint32_t line, uint32_t flags);
    constexpr size_t   kOffModelExtSeqCount  = 0x24; // uint32: sequences whose data streams from .anim
    constexpr size_t   kOffModelExtSeqArray  = 0x28; // -> SMemAlloc'd u32 array (count * 4)
    constexpr size_t   kOffModelExtSeqCursor = 0x2C; // uint32: zeroed by kInit's tail

    // Per-field header readers driven by kInit's offset->pointer walk. Shared cdecl shape:
    //   int Read*(uint8_t* base, uint32_t size, void* header, M2Array* arr)
    // Each validates arr->offset (+ count * stride) against size, then rewrites arr->offset into
    // base + offset (a raw pointer) -- count == 0 zeroes the pointer. The track-bearing readers
    // (bones/colors/transparency/uvanim/attachments/events/lights/ribbons) additionally fix the
    // per-record M2Track outer arrays and, for sequences with flags bit 0x20 (data in-model), the
    // nested per-sequence inner arrays; a global-sequence track fixes all inners unconditionally.
    // ALL of these gate on the rebase global kSeqRebaseState == -1 (load-time walk); the .anim
    // completion path sets it to a sequence index to re-drive only that sequence's inner slots.
    // Strides are IDENTICAL between client 264 and source 272-274 for every reader listed here
    // which is what lets the native
    // MD21 reader drive the stock readers over a modern body. Cameras (0x74 vs 0x64) and particle
    // emitters (0x1EC vs 0x1DC) differ in stride and are NOT listed: the native reader must not
    // call 0x839EF0 / 0x83AF90 on a modern body. All prologues byte-verified against Wow.exe
    // (55 8B EC + "83 3D D8 59 AF 00 FF" cmp of the rebase global).
    constexpr uintptr_t kReadVertices     = 0x00835AE0; // stride 0x30
    constexpr uintptr_t kReadByteArray    = 0x00835B80; // stride 1 (name)
    constexpr uintptr_t kReadVector3      = 0x00835BD0; // stride 0xC (bounding vertices/normals)
    constexpr uintptr_t kReadInt32Array   = 0x00835C20; // stride 4 (global loops, materials)
    constexpr uintptr_t kReadAnimations   = 0x00835C70; // stride 0x40 + looping-id flag post-pass
    constexpr uintptr_t kReadInt16Array   = 0x00835DF0; // stride 2 (every lookup table)
    constexpr uintptr_t kReadTextures     = 0x00836B60; // stride 0x10 + per-texture filename fixup
    constexpr uintptr_t kReadEvents       = 0x00836E40; // stride 0x24 (track base, no values)
    constexpr uintptr_t kReadColors       = 0x00837EE0; // stride 0x28 (color + alpha tracks)
    constexpr uintptr_t kReadTransparency = 0x008382A0; // stride 0x14 (weight track)
    constexpr uintptr_t kReadBones        = 0x008385A0; // stride 0x58 (trans/rot/scale tracks)
    constexpr uintptr_t kReadUVAnimation  = 0x00838B10; // stride 0x3C (trans/rot/scale tracks)
    constexpr uintptr_t kReadAttachments  = 0x00839080; // stride 0x28 (animateAttached track)
    constexpr uintptr_t kReadLights       = 0x00839270; // stride 0x9C (7 tracks)
    // Ribbons (stride 0xB0, identical 264 vs 272-274): the reader IS the already-curated
    // kRibbonDeRelocate (0x83A460); reuse that constant.
    using M2_HeaderReadFn = int(__cdecl*)(uint8_t* base, uint32_t size, void* header, void* array);

    // Load-vs-rebase state global read by every header reader: 0xFFFFFFFF during the load-time walk
    // (fix outer arrays + in-model inners), a sequence index while kPerSeqDeReloc re-drives one
    // streamed sequence. The native reader runs from the kInit detour where it is always -1; the
    // address is curated for asserts/diagnostics only -- never write it.
    constexpr uintptr_t kSeqRebaseState = 0x00AF59D8;

    constexpr uintptr_t kSceneTriangleHitTest = 0x0081D510;
    using M2_SceneTriangleHitTestFn = int(__fastcall*)(
        void* scratch, void* edx, uint16_t* indexBegin, uint16_t* indexEnd, int vertexBase,
        float* point, int mode, int candidate, float* bestDepth, int currentHit);

    // .skin filename builder (pathStem, profileIndex, outBuf): copies the path, strips the extension,
    // appends the two-digit skin suffix. outBuf is a fixed-size engine buffer.
    constexpr uintptr_t kBuildSkinPath = 0x00835A80;
    // Skin-profile loader (model, profileIndex): builds the path, opens+maps the file, parses it,
    // attaches the profile synchronously, and schedules an idempotent async finalize.
    constexpr uintptr_t kLoadSkinProfile = 0x0083CB40;
    // Size of the engine sibling-file path buffer the builders write into.
    constexpr uint32_t  kSkinPathBufSize = 0x108;

    // --- ground-shadow pass (the "shadow swings with the camera" bug) -------------------------
    // CM2Model::RenderModelBatchesForProjectedTexture. NOT a shadow function despite what
    // it projects a DECAL onto M2 bodies (selection
    // ring, pet ring, auto-track cursor, AoE spell reticle). M2s only enter its caster list when the
    // 4th argument of ProjectTex2dDraw carries bit 0, and the shadow callback
    // (World__ProjectTextureCallback 0x0077F500) passes 0 or 2 -- so this is unconditionally dead for
    // shadows in every configuration. Kept because it IS the right hook for decal-on-model work.
    constexpr uintptr_t kProjectedDecalDraw = 0x00829AA0;
    using M2_ProjectedDecalDrawFn = void(__fastcall*)(void* instance, void* edx);

    // The M2 ground shadow is drawn by kRenderBatchShadowMap (0x00829BA0), declared further down --
    // ALREADY HOOKED by features/m2compat/Bones.cpp (oversized-palette guard). Do not install a second
    // detour on it; MinHook returns MH_ERROR_ALREADY_CREATED and the loser silently observes nothing.
    // Chain: CShadowQuery::Render (0x007BBC50) -> CM2Model::RenderModelBatchesShadowMap (0x0082DA40)
    //     -> CM2Model::RenderModelBatchListShadowMap (0x00829E40) -> 0x00829BA0.
    //
    // That path uploads c31 itself from CM2Model+0x98 AND its shader receives
    // c14..c16 = inverse(cameraView) * lightView, which already cancels the camera view the palette
    // carries. So the view/world inconsistency m2_shadow.md blames is NOT the residual-swing cause
    // here -- do not "fix" the palette space on this path without new evidence.
    //
    // CM2Model::RenderModelBatchesShadowMap(opaqueList, alphaList) -- __cdecl, single caller
    // 0x007BC3BA in CShadowQuery::Render. The coarse point for a global shadow on/off; carries no
    // per-instance context (the instance only becomes a register down in 0x00829E40).
    constexpr uintptr_t kShadowMapBatches = 0x0082DA40;

    // CGxDevice::ShaderConstantsUnlock(which, firstConst, constCount) -- __stdcall. For which==0 it
    // only WIDENS the vertex-constant dirty window (min/max); it copies nothing and flushes nothing,
    // so calling it twice over the same range is harmless.
    constexpr uintptr_t kShaderConstUnlock = 0x00683580;
    using Gx_ShaderConstUnlockFn = void(__stdcall*)(int which, unsigned firstConst, int constCount);
    // Vertex-shader constant block base (CGxDevice::ShaderConstantsLock(0) is a pure address lookup
    // returning this) and the bone-palette register c31 = base + 31*16.
    constexpr uintptr_t kVsConstBlock = 0x00C5EFE8;
    constexpr uintptr_t kVsConstC31   = kVsConstBlock + 0x1F0;
    // CShaderEffect::s_enableShaders. Written once by InitShaderSystem from M2GetCacheFlags() & 8, so
    // its value is 8 or 0 -- test for NON-ZERO, never == 1. Zero means the CPU pre-transform path,
    // where the shadow vertices carry no bone data and the palette fix does not apply.
    constexpr uintptr_t kEnableShaders = 0x00D43020;
    // C44Matrix multiply: __cdecl mul(out, a, b) -> out = a*b, row-vector convention (v' = v*M).
    constexpr uintptr_t kMatrixMul = 0x004C1F00;
    using C44_MulFn = float*(__cdecl*)(void* out, const void* a, const void* b);
    // C44Matrix::Inverse -- __thiscall(src in ECX, out on the stack), full general adjoint/determinant
    // inverse. Deliberately NOT 0x004C2FC0 (AffineInverse): that one is rigid-body only (transpose +
    // -t*R) and silently produces a wrong shadow for any placement carrying scale, which doodads do.
    constexpr uintptr_t kMatrixInverse = 0x006A43A0;
    using C44_InverseFn = float*(__thiscall*)(const void* src, void* out);

    // --- particle emitter stride sites ------------------------------------------------------
    // A modern (inner version 272-274) M2 particle emitter record is 0x1EC bytes; the client's is
    // 0x1DC. Every field below 0x1DC is at an IDENTICAL offset in both -- the 16 extra bytes
    // (multiTexScrollMid/Range) are appended at the END. So the client can read a modern record in
    // place; the only thing it gets wrong is how far to step between records.
    //
    // These are every site in the binary that hardcodes the 0x1DC step (verified by scanning all of
    // .text for the dword, not just these instruction forms -- there is no tenth). Each is replaced
    // by a call to a generated thunk that derives the stride from the model being processed, so a
    // scene mixing stock v264 and modern doodads stays correct. Flags are DEAD at all nine sites
    // (each is followed by another flag-setting op before any conditional branch), so the thunks do
    // not need to reproduce OF/CF/ZF.
    //
    // The gate is `header[0x04] > 271`. It is NOT `globalFlags & 0x200`: not one of the 970 modern
    // models in the corpus sets that bit, so testing it would silently yield 0x1DC for every one.
    struct ParticleStrideSite
    {
        uintptr_t va;        ///< instruction address
        uint8_t   length;    ///< 6 or 7 bytes; the thunk call is 5, remainder padded with nop
        uint8_t   headerReg; ///< index into kStrideHeaderSrc below
        uint8_t   opKind;    ///< index into kStrideOp below
        int8_t    disp;      ///< frame displacement for the [ebp+disp] forms, else 0
    };

    // Where the MD20 header pointer lives AT each site, and what the original instruction did.
    // headerReg: 0=[ebp+disp] 1=ebx 2=esi 3=edx 4=ecx 5=edi
    // opKind:    0=imul esi,stride  1=add ecx,stride  2=add ebx,stride  3=add [ebp+disp],stride
    inline constexpr ParticleStrideSite kParticleStrideSites[] = {
        // ReadParticleEmitters 0x0083AF90 -- the BOUNDS CHECK (count*stride + ofs <= fileSize).
        // Header is in memory only here and no register is free; the thunk saves what it uses.
        { 0x0083AFBA, 6, 0, 0,  0x10 },
        // ReadParticleEmitters -- per-record de-relocation cursor. ebx reloaded every iteration.
        { 0x0083AFE7, 6, 1, 0,  0 },
        // CM2Model::InitializeLoaded 0x00832EA0 -- sizing pre-pass. NOTE: this site is a merge point
        // for three branches, so a patch must not assume fall-through reachability.
        { 0x00832F18, 6, 2, 1,  0 },
        // CM2Model::InitializeLoaded -- main emitter-construction loop.
        { 0x00834124, 7, 3, 3, -0x1C },
        // CM2Model::AnimateParticleST 0x008309C0 -- per-frame, single-threaded. x87 state is LIVE.
        { 0x008309EE, 6, 4, 0,  0 },
        // CM2Model::AnimateParticlesMT 0x0082D2F0 -- per-frame, WORKER THREAD. x87 state is LIVE.
        { 0x0082D6BA, 7, 3, 3, -0x0C },
        // CM2Scene::Animate 0x00821A20 -- the ONLY multi-model site: it walks a chain through
        // model->+0x30. ebx holds the CURRENT model's header on every path into the emitter loop
        // (reloaded at 0x00822AC6; the only intervening write is a 7-byte `lea ebx,[ebx]` nop), so
        // deriving per-site is correct here where an entry-detour stride would not be. x87 LIVE.
        { 0x00822CFB, 7, 1, 3, -0x0C },
        // CM2Model::ReplaceTexture 0x00825260 -- on demand.
        { 0x0082536E, 7, 1, 3, -0x04 },
        // CM2Model::ReplaceParticleColor 0x00825410 -- on demand.
        { 0x00825470, 6, 5, 2,  0 },
    };

    constexpr uint32_t kParticleStrideClient = 0x1DC; // v264
    constexpr uint32_t kParticleStrideModern = 0x1EC; // v272-274
    constexpr uint32_t kParticleModernMinVer = 272;   // gate: header[0x04] > 271

    // --- packed multi-texture textureId read sites ------------------------------------------
    // With emitter flag 0x10000000 the textureId at record+0x16 is three packed 5-bit ids, so the raw
    // value (e.g. 5284) is far past the model's texture table. The stock client uses it as a FLAT
    // index and reads out of bounds -- InitializeLoaded then AddRefs table[hugeId] and faults.
    //
    // We teach the client to unpack AT THE READ, leaving the record exactly as the file has it. Each
    // site below is ONE COMPLETE INSTRUCTION replaced by a call to a thunk that redoes that
    // instruction's work and then masks to id1 when the flag is set. Patching a whole instruction (not
    // a multi-instruction window) is what makes this safe: a branch to the instruction's address still
    // lands on our call, and a branch into the middle of an instruction cannot exist in valid code.
    //
    // id2/id3 drive multi-texture particle blending the 3.3.5 renderer has no path for, so id1 is the
    // whole of what this client can consume -- reading it is not a reduction of the data.
    constexpr uintptr_t kParticleTexIdInitLoaded  = 0x00833ED9; // mov ecx,[eax+0x174]  (6 bytes)
    constexpr uint32_t  kParticleTexIdInitLen     = 6;
    constexpr uintptr_t kParticleTexIdReplaceTex  = 0x00825349; // movzx eax,[ecx+edx+0x16] (5 bytes)
    constexpr uint32_t  kParticleTexIdReplaceLen  = 5;
    constexpr uint32_t  kParticleFlagMultiTex     = 0x10000000;

    // CParticleEmitter2::SetZsource -- 39 bytes, exactly two callers (0x00830BFD in AnimateParticleST,
    // 0x00833CF3 in InitializeLoaded). Modern content stores zSource = 255.0 as a DISABLED sentinel,
    // but CPlaneParticleEmitter::CreateParticle (0x009815C0) tests `zSource == 0.0` and treats any
    // other value as a real point source: it discards the verticalRange/horizontalRange emission cone
    // and overwrites the spawn position with a normalized (pos - (0,0,zSource)) vector. That is why
    // modern fire drifts sideways instead of rising. 1179 of 2528 emitters in the corpus carry 255.0.
    constexpr uintptr_t kSetZsource = 0x00978DA0;
    using M2_SetZsourceFn = void(__fastcall*)(void* emitter, void* edx, float zSource);

    // --- external animation ---
    // External-anim read-completion callback (node): runs once after the bytes are read and before the
    // per-sequence track offsets are rebased.
    constexpr uintptr_t kAnimLoadComplete = 0x0083D840;
    // External-anim loader (model, seqIdx): resolves the sequence alias chain, builds the path, opens
    // the file, allocates a buffer, and schedules the async read whose completion rebases the tracks.
    constexpr uintptr_t kSequenceLoad = 0x0083DA10;
    // .anim filename builder (pathStem, id, subId, outBuf): copies the stem, strips the extension,
    // appends the id-subId anim suffix.
    constexpr uintptr_t kBuildAnimPath = 0x00835A20;
    // Per-sequence track de-relocator (model, seqIdx, buffer, size): validates the buffer and rebases
    // sequence seqIdx's track inner slots against it, then updates the sequence flags.
    constexpr uintptr_t kPerSeqDeReloc = 0x0083C6E0;
    // M2 buffer allocator (size, name, line): allocates size+0x10, returns a 16-aligned pointer carrying a
    // back-shift byte at [ptr-1]. This is the allocator the .m2 load buffer (model+0x150) uses, so a
    // replacement buffer must come from here for the model destructor's matching free to be valid.
    constexpr uintptr_t kAnimBufferAlloc = 0x0083DE50;
    constexpr uintptr_t kBufferAlloc     = 0x0083DE50; // alias: same allocator, used for buffer swaps
    constexpr uintptr_t kBufferFree      = 0x0083DE90; // free a kBufferAlloc pointer (recovers base via [ptr-1])
    using M2_BufferAllocFn = void*(__cdecl*)(uint32_t size, const char* tag, int line);
    using M2_BufferFreeFn  = void (__cdecl*)(void* ptr);

    // I/O record field offsets used by the rebase: the buffer base and its byte size.
    constexpr size_t kOffRecordBuffer = 0x04;
    constexpr size_t kOffRecordSize   = 0x08;
    // Load-node field: the I/O record pointer.
    constexpr size_t kOffNodeRecord   = 0x08;

    // --- per-batch alpha ---
    // Shared per-batch alpha/material/cull setter: chooses the alpha-test reference from the blend mode
    // and pushes it to the device.
    constexpr uintptr_t kSetupBatchAlpha = 0x0081FE90;
    constexpr uintptr_t kSortOpaqueGeoBatches = 0x0081EAD0;
    // Pushes the alpha-test reference to the device.
    constexpr uintptr_t kPushAlphaRef = 0x00873BA0;

    // SetupBatchAlpha draw-context fields (this = the draw context): the instance being drawn and the live
    // material the caller set. Instance -> model is kOffInstModel below.
    constexpr size_t kOffDrawCtxInstance = 0x60; // draw context -> render instance
    constexpr size_t kOffDrawCtxMaterial = 0x98; // draw context -> live material record
    // Material record: the blend mode (1 = alpha key).
    constexpr size_t kOffMaterialBlend   = 0x02;

    // --- bone palette (per-frame skinning matrices; the bone-physics hook point) ---
    // Per-instance bone-palette build (instance, ...): fills the bone matrices for one instance from
    // the current pose, each frame, before the batch draw uploads the palette to the vertex shader.
    // Called from two sites per collection model per frame:
    //   (a) sub_8309C0 (kUpdateAttachedModels, called from inside kM2PerFrameUpdate of the parent)
    //   (b) the outer scene-traversal loop at 0x821B4E (iterates the full scene linked list)
    // Site (b) fires AFTER the parent's kM2PerFrameUpdate completes (and therefore after the
    // OnM2PerFrameUpdate event). Hooking kBuildBonePalette and re-applying the CharSweep in the
    // POST-hook guarantees the bone copy is the last write before GPU upload, regardless of
    // scene-list ordering. Both sites use fastcall (ecx = instance) with 5 stack args; ret 0x14.
    constexpr uintptr_t kBuildBonePalette = 0x0082F0F0;
    // Single-bone fast-path palette build: same call shape (fastcall, 5 stack args, ret 0x14).
    // Instances with init-flag bit 0x1000 route here; the build is one matrix copy, so the cadence
    // skip never applies to it and the recomposition walk calls it directly for such children.
    constexpr uintptr_t kBuildBonePaletteSimple = 0x0082E140;
    constexpr uintptr_t kRenderBatchShadowMap = 0x00829BA0;

    // Return addresses of the scene walk's per-model palette-build calls. Cadence skipping engages
    // only when the build was invoked from one of these three sites; every other caller (on-demand
    // rebuilds after a sequence change, attachment recursion, particle spawn models) expects a
    // fresh full pose and must always get one.
    constexpr uintptr_t kPaletteCallRetSceneA = 0x00821B53; // frame driver walk (also threaded main half)
    constexpr uintptr_t kPaletteCallRetSceneB = 0x00821BDD; // frame driver walk, second branch
    constexpr uintptr_t kPaletteCallRetWorker = 0x0081CEFF; // worker thread's half of the interleaved walk

    // --- track evaluators (sampled per bone / per light each frame from the bone-palette build) ---
    // Vec3 track evaluator (model, runtimeBone, track, out, baseValue): samples a translation/scale
    // track for the current animation index and writes the interpolated vec3 into out.
    constexpr uintptr_t kTrackEvalVec3 = 0x0082B0A0;
    // Quaternion track evaluator (model, runtimeBone, track, out, baseValue): samples a rotation track
    // for the current animation index and writes the interpolated quaternion into out.
    constexpr uintptr_t kTrackEvalQuat = 0x00828680;

    // --- matrix/vector helpers the palette build routes its math through -----------------------
    // The recomposition walk calls these same entries so a recomposed palette is bit-identical to a
    // fully built one; only a handful of scalar expressions are ever evaluated outside them.
    // Rotation-matrix build from a quaternion: this = destination matrix, one stack arg (quat).
    // Writes all 16 elements (row 3 = 0,0,0,1).
    constexpr uintptr_t kMatrixFromQuat = 0x004C1DE0;
    using C44_FromQuatFn = void*(__thiscall*)(void* mat, const float* quat);
    // Row scale: rows 0..2 multiplied component-wise by v.x / v.y / v.z.
    constexpr uintptr_t kMatrixScaleRows = 0x004C1B90;
    using C44_ScaleRowsFn = void(__thiscall*)(void* mat, const float* vec3);
    // Pre-translate: folds a translation (applied before the matrix) into the translation row.
    constexpr uintptr_t kMatrixPreTranslate = 0x004C1B30;
    using C44_PreTranslateFn = void(__thiscall*)(void* mat, const float* vec3);
    // In-place multiply: this = this * other (row-vector convention).
    constexpr uintptr_t kMatrixMulAssign = 0x004C2370;
    using C44_MulAssignFn = void(__thiscall*)(void* mat, const void* other);
    // Affine point transform: out = vec * mat, including the translation row. cdecl, returns out.
    constexpr uintptr_t kVec3Transform = 0x004C21B0;
    using C3_TransformFn = float*(__cdecl*)(float* out, const float* vec3, const void* mat);
    // In-place 3-component normalize.
    constexpr uintptr_t kVec3Normalize = 0x004C3600;
    using C3_NormalizeFn = void(__thiscall*)(float* vec3);

    // --- ribbon ---
    // Ribbon-emitter de-relocator: pointer-fixes each ribbon emitter's sub-array offsets.
    constexpr uintptr_t kRibbonDeRelocate = 0x0083A460;
    // M2ModelHeader::ReadParticleEmitters -- same (base, fileSize, header, arrayField) shape as the
    // other kRead* walkers. Safe on a modern body ONLY once features/m2native/ParticleStride.cpp has
    // redirected the two 0x1DC stride immediates it contains (0x0083AFBA bounds check, 0x0083AFE7
    // cursor); until then it would validate against a short size and then walk off the array.
    constexpr uintptr_t kReadParticleEmitters = 0x0083AF90;
    // Ribbon emitter draw (emitter, stateBlock): builds the strip and binds one texture per layer.
    constexpr uintptr_t kRibbonDraw = 0x00980B70;
    // Resolve a texture handle to the internal texture object the sampler bind expects.
    constexpr uintptr_t kTexResolve = 0x004B6CB0;
    // Bind a texture to a sampler selector (device, selector, resolvedTexture).
    constexpr uintptr_t kSamplerBind = 0x00685F50;
    // Sampler selectors for the engine bind path: s0 = 0x15, consecutive. The native ribbon loop binds
    // only s0; the extra layers of a multi-texture ribbon are bound to s1/s2 so they survive one pass.
    constexpr uint32_t kSamplerSelS1 = 0x16;
    constexpr uint32_t kSamplerSelS2 = 0x17;

    // --- attachment / render-context functions ---
    // GetRenderCtx(cmo, keyBuf): returns the per-model render context for the given CharModelObject
    // and key buffer; allocates one if absent.
    constexpr uintptr_t kGetRenderCtx       = 0x0081F8F0;
    // AttachToScene(renderCtx, subObj, slot): attaches a collection-M2 render context to a scene slot
    // on the parent CharModelObject render context.
    constexpr uintptr_t kAttachToScene      = 0x00831630;
    // DetachSlot(subObj, slot): detaches the M2 bound to a scene slot, releasing its render context.
    constexpr uintptr_t kDetachSlot         = 0x00827560;
    // ReleaseRenderCtx(renderCtx): releases a render context obtained from GetRenderCtx.
    constexpr uintptr_t kReleaseRenderCtx   = 0x00824ED0;
    // BindTexSlot(renderCtx, modelPtr): binds the M2 model resource to texture slot key 2 (main texture).
    constexpr uintptr_t kBindTexSlot        = 0x00825260;
    // LoadResource(path, flags): loads a texture/resource by virtual path through the texture-create path.
    constexpr uintptr_t kLoadResource       = 0x004B9760;
    // ReleaseResource(resource): releases a resource handle returned by LoadResource.
    constexpr uintptr_t kReleaseResource    = 0x0047BF30;

    // --- character-model slot hooks ---
    // Per-render-ctx per-frame update: fires once per visible M2 instance per frame, recursively
    // through the scene graph. Hooked to drive bone-matrix copy and geoset filtering.
    constexpr uintptr_t kM2PerFrameUpdate      = 0x00828A00;
    // CharModel equip-slot handler (cmo, modelSlot, itemDataPtr, postFlag): dispatches an item to
    // an internal model slot, building paths and loading the M2.
    constexpr uintptr_t kCharModelSlotDispatch = 0x004F2640;
    // CharModel equip-slot clear (cmo, equipSlotWow): clears the WoW equipment slot on the CMO,
    // detaching any attached M2 and releasing its render context.
    constexpr uintptr_t kCharModelSlotClear    = 0x004EE6D0;

    // --- runtime instance object fields ---
    // Low bit set on a PARENT instance: its children derive their own view distance instead of
    // inheriting the parent's (read by the palette build's distance step).
    constexpr size_t kOffInstOwnerFlags     = 0x04;
    constexpr size_t kOffInstInitFlags      = 0x10;  // init flags (bit 0 = anim init done; bit 6 = char-select present)
    constexpr size_t kOffInstModel          = 0x2C;  // -> runtime model
    constexpr size_t kOffInstScene          = 0x28;  // -> the CM2Scene this instance belongs to
    constexpr size_t kOffInstCmdRingHead    = 0x34;  // deferred-command ring cursor; equal to the tail when idle
    constexpr size_t kOffInstCmdRingTail    = 0x38;
    constexpr size_t kOffInstLastAnimFrame  = 0x3C;  // uint32: scene frame this instance last animated on
    constexpr size_t kOffSceneClock         = 0x0C;  // uint32 on the scene: absolute animation clock (ms)
    constexpr size_t kOffSceneFrame         = 0x14;  // uint32 on CM2Scene: the current frame counter
    constexpr size_t kOffInstParent         = 0x48;  // -> parent M2 instance (null for root)
    constexpr size_t kOffInstAttachEnable   = 0x4C;  // -> per-attachment enable records (stride 0xC, u8 at +0x8)
    constexpr size_t kOffInstAttachSlot     = 0x54;  // uint32: attachment index this instance hangs on (0xFFFF = none)
    constexpr size_t kOffInstAttachedHead   = 0x58;  // -> first attached child instance
    constexpr size_t kOffInstAttachedNext   = 0x60;  // -> next sibling in the parent's attached-child list
    constexpr size_t kOffInstFreezeAnchor   = 0x64;  // uint32: nonzero arms externally driven pose freezing
    constexpr size_t kOffInstViewDistSq     = 0x88;  // float: squared view-space distance (also the draw sort key)
    constexpr size_t kOffInstConstTrackGate = 0x90;  // uint32: once-only constant-track sampling gate
    constexpr size_t kOffInstBoneStates     = 0x94;  // -> runtime bone-state block (stride kRuntimeBoneStride)
    constexpr size_t kOffInstTexBinding     = 0x174; // shared texture-binding slot mirrored from the parent
    constexpr size_t kOffInstSpeedBase      = 0x178; // float: base playback speed
    constexpr size_t kOffInstAlphaBase      = 0x17C; // float: base alpha factor
    constexpr size_t kOffInstScaleBase      = 0x180; // float[3]: base scale
    constexpr size_t kOffInstTransBase      = 0x18C; // float[3]: base translation
    constexpr size_t kOffInstSpeedStage     = 0x198; // float: staged speed consumed by children/particles
    constexpr size_t kOffInstAlphaStage     = 0x19C; // float: staged alpha
    constexpr size_t kOffInstScaleStage     = 0x1A0; // float[3]: staged scale passed into child recursion
    constexpr size_t kOffInstTransStage     = 0x1AC; // float[3]: staged translation

    // Init-flag bits (kOffInstInitFlags) the palette build and its cadence logic consult.
    constexpr uint32_t kInstFlagLive       = 0x1;      // instance participates in animation at all
    constexpr uint32_t kInstFlagAnimDirty  = 0x400;    // pending state change; cleared by the full build tail
    constexpr uint32_t kInstFlagSimplePath = 0x1000;   // routed to the single-bone fast-path build
    constexpr uint32_t kInstFlagChildBase  = 0x40000;  // unslotted child rides the parent's palette slot 0
    constexpr uint32_t kInstFlagFixedTrans = 0x80000;  // staged translation ignores the caller's offset
    constexpr uint32_t kInstFlagFixedSpeed = 0x100000; // staged speed ignores the caller's multiplier
    constexpr size_t kOffInstBonePalette    = 0x98;  // -> bone matrices, row-major 4x4
    constexpr size_t kBonePaletteStride     = 0x40;  // one bone matrix
    // Model->world placement, an INLINE 64-byte C44Matrix (the shadow loop reads &instance+0xb4).
    constexpr size_t kOffInstPlacement      = 0xB4;
    // Instance root, an INLINE 64-byte C44Matrix written by CM2Model::AnimateMT (0x0082F0F0) as
    // placement * viewRoot -- i.e. VIEW space. Every palette slot is B_bone * this, so the model-space
    // factor of a bone is palette[b] * inverse(this) (palette FIRST: row-vector convention).
    constexpr size_t kOffInstViewRoot       = 0xF4;
    // Array of render-object pointers (4 bytes each), indexed by M2SkinProfile batch index.
    // Read by sub_828A00 to call sub_97f570 which controls per-batch draw visibility.
    constexpr size_t kOffInstRenderObjArray = 0x2BC; // -> void*[] (one pointer per skin batch)
    // -> per-instance batch/section override arrays ([0] = batches, [2] = sections). When non-null,
    // CM2Model::RenderModelBatchListShadowMap (0x00829E40) resolves the shadow draw's section from
    // here INSTEAD of the CM2Shared+0x18C runtime copy -- so any fixup applied to that copy (notably
    // the boneInfluences 0 -> 1 lift at skin finalize) is bypassed for such an instance.
    constexpr size_t kOffInstSectionOverride = 0x2D0;
    // Visibility flags written by sub_97f570 into each render object.
    // Bit 2 = 1 → batch visible; bit 2 = 0 → batch hidden (bit 0 also cleared when hiding).
    constexpr size_t kOffRenderObjFlags     = 0x160;

    // --- runtime model object fields ---
    constexpr size_t kOffModelFlags         = 0x08;  // bit 2 selects the sibling-file open flag
    constexpr size_t kOffModelPathStem      = 0x3C;  // model path stem (no extension)
    constexpr size_t kOffModelHeader        = 0x150; // -> raw .m2 file buffer (parsed in place -> becomes the header)
    constexpr size_t kOffModelFileSize      = 0x16C; // byte size of the .m2 file buffer at +0x150
    constexpr size_t kOffModelSkin          = 0x170; // -> live parsed skin profile (valid at/after skin finalize)
    constexpr size_t kOffModelSubMeshCopy   = 0x188; // -> per-submesh object array ptr (written by kFinalizeSkin; not freed on re-call)
    constexpr size_t kOffModelSubmeshBuf    = 0x18C; // -> submesh copy buffer ptr (written by kFinalizeSkin; not freed on re-call)
    constexpr size_t kOffModelFinalizeDone  = 0x190; // uint32: set to 1 by kFinalizeSkin after IB is built; scheduler checks before re-calling
    // uint16: count of bones carrying flags & 0x2F8 (0x200 "transformed" + the billboard bits 0xF8),
    // tallied by CM2Shared::Initialize (0x0083CC80). Its ONLY reader is CM2Model::AnimateSM
    // (0x00831990), the SHADOW-path animate, which rebuilds the bone palette at instance+0x98 only
    // when `instance->parent != 0 || this != 0`; otherwise it takes a fast path that leaves the
    // palette holding an OLDER frame's view matrix. CShadowQuery::Render meanwhile uploads
    // c14..c16 = inverse(CURRENT view) * lightView, so a stale palette no longer cancels and the
    // shadow rotates by the camera's motion since that frame. WotLK exporters bake 0x200 into
    // practically every bone, so stock content always takes the full path; Legion/Midnight ship all
    // bone flags 0x0 and would take the fast path -- hence the swing.
    constexpr size_t kOffSharedAnimGateCount = 0x198;
    // uint32: MAX INSTANCES per batched draw, computed by kFinalizeSkin as
    //   min(65536 / skin->indices.count, min over submeshes of skin->boneCountMax / submesh.boneCount), floored at 1
    // i.e. how many copies of the model fit in one 16-bit index buffer AND in the bone-constant palette.
    // Consumers: CM2Shared::AllocInstances (0x00836DF0) clamps its grow request to it, and
    // CM2Model::InitializeLoaded (0x00832EA0) clears the "batchable doodad" flag 0x10 when it is < 2
    // (cf. CVar M2BatchDoodads). NOTHING here is level-of-detail: 12340 has no per-frame M2 LOD at all.
    constexpr size_t kOffSharedMaxInstances = 0x194;
    // Deprecated spelling kept so no published name disappears; the "LodMultiplier" reading was wrong.
    constexpr size_t kOffModelLodMultiplier = kOffSharedMaxInstances;

    // --- parsed file-header fields ---
    constexpr size_t kOffHdrGlobalFlags    = 0x10; // bit 0x20 = model carries physics
    // Global-flag bit 0x4: the staged speed/scale/translation ignore the caller's arguments and
    // come from the instance's base fields alone (the palette build's alternate staging branch).
    constexpr uint32_t kHdrFlagFixedStaging = 0x4;
    constexpr size_t kOffHdrSeqCount       = 0x1C; // sequence records
    constexpr size_t kOffHdrSeqPtr         = 0x20; // -> sequence records (stride kSeqStride)
    constexpr size_t kSeqStride            = 0x40;
    constexpr size_t kOffSeqFlags          = 0x0C; // bit 0x1 = plays once to its end, then holds
    constexpr size_t kOffHdrBoneCount      = 0x2C;
    constexpr size_t kOffHdrBoneArray      = 0x30; // -> bone records (post-fixup data ptr)
    constexpr size_t kOffHdrAttachCount    = 0xF0; // attachment records
    constexpr size_t kOffHdrAttachPtr      = 0xF4; // -> attachment records (stride kAttachStride)
    constexpr size_t kAttachStride         = 0x28;
    constexpr size_t kOffAttachBone        = 0x04; // uint16: palette slot a child attached here rides
    constexpr size_t kOffAttachPos         = 0x08; // float[3]: offset applied to that slot
    // boneCombos M2Array {count, offset}: the per-section bone-index window the palette upload walks.
    // After the load walk the offset field holds a real pointer, hence the separate ...Ptr spelling.
    constexpr size_t kOffHdrBoneCombosCount = 0x78;
    constexpr size_t kOffHdrBoneCombosPtr   = 0x7C; // -> uint16 array
    constexpr size_t kOffHdrBoneIdxLutCount= 0xF8; // count of bone-index-by-id LUT entries
    constexpr size_t kOffHdrBoneIdxLutPtr  = 0xFC; // -> bone-index-by-id LUT (uint16 array, indexed by key_bone_id)

    // --- bone record fields (in the header bone array) ---
    constexpr size_t   kBoneStride        = 0x58;
    constexpr size_t   kOffBoneKeyId      = 0x00; // key_bone_id: canonical slot id (negative = none)
    constexpr size_t   kOffBoneFlags      = 0x04; // bone flags
    constexpr size_t   kOffBoneParent     = 0x08; // int16 parent index (0xFFFF = root)
    constexpr size_t   kOffBoneNameCrc    = 0x0C; // CRC32 of the bone name string (for name-based remap)
    constexpr size_t   kOffBoneTransTrack = 0x10; // translation track head (kOffTrackTimestampsCount = outer slots)
    constexpr size_t   kOffBoneRotTrack   = 0x24; // rotation track head
    constexpr size_t   kOffBoneScaleTrack = 0x38; // scale track head
    constexpr size_t   kOffBonePivot      = 0x4C; // pivot (bone origin in bind space)
    constexpr uint32_t kBoneBillboardMask = 0x78; // spherical + cylindrical-lock bits
    // Effective bone flags = runtime override mask OR'd with the static flags. Masks the build tests:
    constexpr uint32_t kBoneIgnoreParentMask = 0x07;  // rebuild against the instance root instead of the parent
    constexpr uint32_t kBoneAnimatedMask     = 0x280; // bone carries its own local transform
    constexpr uint32_t kBoneProceduralFlag   = 0x80;  // multiplies in the externally supplied matrix

    // --- runtime bone-state block (per bone, stride kRuntimeBoneStride) ---
    // The persisted sampling records the recomposition walk reads instead of re-sampling tracks;
    // the anchor/sequence fields feed the cadence eligibility scan and its mutation fingerprint.
    constexpr size_t kRuntimeBoneStride     = 0xAC;
    constexpr size_t kOffRtBoneTransValue   = 0x08; // float[3]: last sampled translation
    constexpr size_t kOffRtBoneQuatValue    = 0x1C; // float[4]: last sampled rotation quaternion
    constexpr size_t kOffRtBoneScaleValue   = 0x34; // float[3]: last sampled scale
    constexpr size_t kOffRtBoneAssignedSeq  = 0x48; // uint16: assigned sequence (0xFFFF = inherit)
    constexpr size_t kOffRtBoneSeqStart     = 0x4C; // int32: sequence start anchor (absolute clock ms)
    constexpr size_t kOffRtBoneSeqEnd       = 0x50; // int32: sequence end (absolute clock ms)
    constexpr size_t kOffRtBoneTimeScale    = 0x54; // float: playback speed factor
    constexpr size_t kOffRtBoneTimeOffset   = 0x5C; // int32: offset into the sequence
    constexpr size_t kOffRtBoneBlendSeq     = 0x6C; // uint16: blend-from sequence (0xFFFF = no blend)
    constexpr size_t kOffRtBoneProcMatrix   = 0x88; // -> externally driven per-frame matrix (null = none)
    constexpr size_t kOffRtBoneFlagMask     = 0x8C; // uint32: runtime bone-flag override mask
    constexpr size_t kOffRtBonePendingSeq   = 0x90; // uint32: pending sequence bookkeeping (0xFFFFFFFF idle)
    constexpr size_t kOffRtBoneBlendWeight  = 0xA8; // float: current blend weight (0 = none)

    // --- CharModelObject fields ---
    constexpr size_t kOffCmoRace      = 0x18; // uint32 race id
    constexpr size_t kOffCmoGender    = 0x1C; // uint32 gender (0 = male, 1 = female)
    constexpr size_t kOffCmoSceneNode = 0x38; // -> SceneNode (the root scene node for this character)

    // --- SceneNode fields ---
    constexpr size_t kOffSceneNodeOwner = 0x28; // -> CharModelObject that owns this scene node

    // --- track object fields read by the evaluators ---
    constexpr size_t kOffTrackTimestampsCount = 0x04;
    constexpr size_t kOffTrackTimestampsPtr   = 0x08;
    constexpr size_t kOffTrackValuesCount     = 0x0C;
    constexpr size_t kOffTrackValuesPtr       = 0x10;
    constexpr size_t kTrackTimestampStride    = 0x04; // one timestamp = u32 ms
    constexpr size_t kTrackVec3Stride         = 0x0C; // one vec3 value = 3 floats
    constexpr size_t kTrackQuatStride         = 0x08; // one compressed quat = 4 int16
    // Runtime-bone field: the current animation index used to pick the per-animation inner slot.
    constexpr size_t kOffRuntimeBoneAnimIdx   = 0x44;

    // --- ribbon-emitter object fields ---
    constexpr size_t kOffRibbonLayerCount   = 0x118; // draw-loop bound
    constexpr size_t kOffRibbonTexHandlePtr = 0x12C; // -> per-layer texture-handle array (stride 4)

    // --- typed views over the objects above ---
    // The constants are the curated landmarks; these structs give named, typed access to the same fields,
    // with every member offset checked against a constant at compile time (a wrong padding fails the build).
    // Only RE'd fields are named; the gaps are explicit padding. Pointers are 4 bytes on the 32-bit client.
#pragma pack(push, 1)
    /** @brief I/O record read by the per-sequence rebase: the loaded buffer base and its byte size. */
    struct IoRecord
    {
        uint8_t  _pad00[kOffRecordBuffer];
        void*    buffer;           // kOffRecordBuffer (loaded bytes base)
        uint32_t size;             // kOffRecordSize (byte count)
    };
    static_assert(offsetof(IoRecord, buffer) == kOffRecordBuffer, "IoRecord.buffer");
    static_assert(offsetof(IoRecord, size)   == kOffRecordSize,   "IoRecord.size");

    /** @brief Async load node: holds the I/O record pointer. */
    struct LoadNode
    {
        uint8_t  _pad00[kOffNodeRecord];
        void*    record;           // kOffNodeRecord -> IoRecord
    };
    static_assert(offsetof(LoadNode, record) == kOffNodeRecord, "LoadNode.record");

    /** @brief SetupBatchAlpha draw context: the instance being drawn and the live material. */
    struct DrawContext
    {
        uint8_t  _pad00[kOffDrawCtxInstance];
        void*    instance;         // kOffDrawCtxInstance -> M2Instance
        uint8_t  _pad64[kOffDrawCtxMaterial - (kOffDrawCtxInstance + sizeof(void*))];
        void*    material;         // kOffDrawCtxMaterial -> Material
    };
    static_assert(offsetof(DrawContext, instance) == kOffDrawCtxInstance, "DrawContext.instance");
    static_assert(offsetof(DrawContext, material) == kOffDrawCtxMaterial, "DrawContext.material");

    /** @brief Live material record: the blend mode the draw uses to pick the alpha-test reference. */
    struct Material
    {
        uint8_t  _pad00[kOffMaterialBlend];
        uint16_t blend;            // kOffMaterialBlend (1 = alpha key)
    };
    static_assert(offsetof(Material, blend) == kOffMaterialBlend, "Material.blend");

    /** @brief Runtime instance (= render context wrapper returned by GetRenderCtx).
     *         Both the character's scene node (cmo+0x38) and collection M2 render contexts
     *         share this layout. Covers the fields the per-frame palette build and its cadence
     *         logic touch: flags, links, anchors, the bone-state/palette pointers, the inline
     *         placement/root matrices and the staged speed/scale/translation block. */
    struct M2Instance
    {
        uint8_t  _pad00[kOffInstInitFlags];
        uint32_t initFlags;        // kOffInstInitFlags (kInstFlag* bits)
        uint8_t  _pad14[kOffInstScene - (kOffInstInitFlags + sizeof(uint32_t))];
        void*    scene;            // kOffInstScene -> owning scene
        void*    model;            // kOffInstModel -> M2Model (shared model object)
        uint8_t  _pad30[kOffInstCmdRingHead - (kOffInstModel + sizeof(void*))];
        uint32_t cmdRingHead;      // kOffInstCmdRingHead (== cmdRingTail when no deferred commands)
        uint32_t cmdRingTail;      // kOffInstCmdRingTail
        uint32_t lastAnimFrame;    // kOffInstLastAnimFrame (0 = never posed)
        uint8_t  _pad40[kOffInstParent - (kOffInstLastAnimFrame + sizeof(uint32_t))];
        void*    parent;           // kOffInstParent -> native parent M2 instance
        void*    attachEnable;     // kOffInstAttachEnable -> enable records (stride 0xC, u8 at +0x8)
        uint8_t  _pad50[kOffInstAttachSlot - (kOffInstAttachEnable + sizeof(void*))];
        uint32_t attachSlot;       // kOffInstAttachSlot (0xFFFF = not slot-attached)
        void*    attachedHead;     // kOffInstAttachedHead -> first attached child
        uint8_t  _pad5c[kOffInstAttachedNext - (kOffInstAttachedHead + sizeof(void*))];
        void*    attachedNext;     // kOffInstAttachedNext -> next sibling under the same parent
        uint32_t freezeAnchor;     // kOffInstFreezeAnchor (nonzero = externally frozen pose)
        uint8_t  _pad68[kOffInstViewDistSq - (kOffInstFreezeAnchor + sizeof(uint32_t))];
        float    viewDistSq;       // kOffInstViewDistSq
        uint8_t  _pad8c[kOffInstConstTrackGate - (kOffInstViewDistSq + sizeof(float))];
        uint32_t constTrackGate;   // kOffInstConstTrackGate
        void*    boneStates;       // kOffInstBoneStates -> RuntimeBone[boneCount]
        void*    bonePalettePtr;   // kOffInstBonePalette -> heap bone-matrix buffer (row-major 4x4, kBonePaletteStride each)
        uint8_t  _pad9c[kOffInstPlacement - (kOffInstBonePalette + sizeof(void*))];
        float    placement[16];    // kOffInstPlacement (inline model->world matrix)
        float    viewRoot[16];     // kOffInstViewRoot (inline placement * view matrix)
        uint8_t  _pad134[kOffInstTexBinding - (kOffInstViewRoot + 16 * sizeof(float))];
        void*    texBinding;       // kOffInstTexBinding (mirrored from the parent each build)
        float    speedBase;        // kOffInstSpeedBase
        float    alphaBase;        // kOffInstAlphaBase
        float    scaleBase[3];     // kOffInstScaleBase
        float    transBase[3];     // kOffInstTransBase
        float    speedStage;       // kOffInstSpeedStage
        float    alphaStage;       // kOffInstAlphaStage
        float    scaleStage[3];    // kOffInstScaleStage
        float    transStage[3];    // kOffInstTransStage
    };
    static_assert(offsetof(M2Instance, initFlags)     == kOffInstInitFlags,     "M2Instance.initFlags");
    static_assert(offsetof(M2Instance, scene)         == kOffInstScene,         "M2Instance.scene");
    static_assert(offsetof(M2Instance, model)         == kOffInstModel,         "M2Instance.model");
    static_assert(offsetof(M2Instance, cmdRingHead)   == kOffInstCmdRingHead,   "M2Instance.cmdRingHead");
    static_assert(offsetof(M2Instance, cmdRingTail)   == kOffInstCmdRingTail,   "M2Instance.cmdRingTail");
    static_assert(offsetof(M2Instance, lastAnimFrame) == kOffInstLastAnimFrame, "M2Instance.lastAnimFrame");
    static_assert(offsetof(M2Instance, parent)        == kOffInstParent,        "M2Instance.parent");
    static_assert(offsetof(M2Instance, attachEnable)  == kOffInstAttachEnable,  "M2Instance.attachEnable");
    static_assert(offsetof(M2Instance, attachSlot)    == kOffInstAttachSlot,    "M2Instance.attachSlot");
    static_assert(offsetof(M2Instance, attachedHead)  == kOffInstAttachedHead,  "M2Instance.attachedHead");
    static_assert(offsetof(M2Instance, attachedNext)  == kOffInstAttachedNext,  "M2Instance.attachedNext");
    static_assert(offsetof(M2Instance, freezeAnchor)  == kOffInstFreezeAnchor,  "M2Instance.freezeAnchor");
    static_assert(offsetof(M2Instance, viewDistSq)    == kOffInstViewDistSq,    "M2Instance.viewDistSq");
    static_assert(offsetof(M2Instance, constTrackGate)== kOffInstConstTrackGate,"M2Instance.constTrackGate");
    static_assert(offsetof(M2Instance, boneStates)    == kOffInstBoneStates,    "M2Instance.boneStates");
    static_assert(offsetof(M2Instance, bonePalettePtr)== kOffInstBonePalette,   "M2Instance.bonePalettePtr");
    static_assert(offsetof(M2Instance, placement)     == kOffInstPlacement,     "M2Instance.placement");
    static_assert(offsetof(M2Instance, viewRoot)      == kOffInstViewRoot,      "M2Instance.viewRoot");
    static_assert(offsetof(M2Instance, texBinding)    == kOffInstTexBinding,    "M2Instance.texBinding");
    static_assert(offsetof(M2Instance, speedBase)     == kOffInstSpeedBase,     "M2Instance.speedBase");
    static_assert(offsetof(M2Instance, alphaBase)     == kOffInstAlphaBase,     "M2Instance.alphaBase");
    static_assert(offsetof(M2Instance, scaleBase)     == kOffInstScaleBase,     "M2Instance.scaleBase");
    static_assert(offsetof(M2Instance, transBase)     == kOffInstTransBase,     "M2Instance.transBase");
    static_assert(offsetof(M2Instance, speedStage)    == kOffInstSpeedStage,    "M2Instance.speedStage");
    static_assert(offsetof(M2Instance, alphaStage)    == kOffInstAlphaStage,    "M2Instance.alphaStage");
    static_assert(offsetof(M2Instance, scaleStage)    == kOffInstScaleStage,    "M2Instance.scaleStage");
    static_assert(offsetof(M2Instance, transStage)    == kOffInstTransStage,    "M2Instance.transStage");

    /** @brief Scene clock/frame block read by the per-frame build and its cadence decisions. */
    struct M2SceneClock
    {
        uint8_t  _pad00[kOffSceneClock];
        uint32_t clock;            // kOffSceneClock (absolute animation clock, ms)
        uint32_t delta;            // this frame's clock delta
        uint32_t frame;            // kOffSceneFrame
    };
    static_assert(offsetof(M2SceneClock, clock) == kOffSceneClock, "M2SceneClock.clock");
    static_assert(offsetof(M2SceneClock, frame) == kOffSceneFrame, "M2SceneClock.frame");

    /** @brief Runtime model: flags, path stem, the parsed .m2 buffer, its size, and the live skin profile. */
    struct M2Model
    {
        uint8_t  _pad00[kOffModelFlags];
        uint32_t flags;            // kOffModelFlags (bit 2 selects the sibling-file open flag)
        uint8_t  _pad0c[kOffModelPathStem - (kOffModelFlags + sizeof(uint32_t))];
        char     pathStem[kOffModelHeader - kOffModelPathStem]; // kOffModelPathStem (inline path stem, no extension)
        void*    header;           // kOffModelHeader -> raw .m2 file buffer (parsed in place)
        uint8_t  _pad154[kOffModelFileSize - (kOffModelHeader + sizeof(void*))];
        uint32_t fileSize;         // kOffModelFileSize (byte size of the buffer at kOffModelHeader)
        void*    skin;             // kOffModelSkin -> live parsed skin profile
    };
    static_assert(offsetof(M2Model, flags)    == kOffModelFlags,    "M2Model.flags");
    static_assert(offsetof(M2Model, pathStem) == kOffModelPathStem, "M2Model.pathStem");
    static_assert(offsetof(M2Model, header)   == kOffModelHeader,   "M2Model.header");
    static_assert(offsetof(M2Model, fileSize) == kOffModelFileSize, "M2Model.fileSize");
    static_assert(offsetof(M2Model, skin)     == kOffModelSkin,     "M2Model.skin");

    /** @brief Parsed file header: the global flags, sequence/bone/attachment arrays, and the
     *         bone-index-by-id LUT. */
    struct M2FileHeader
    {
        uint8_t  _pad00[kOffHdrGlobalFlags];
        uint32_t globalFlags;       // kOffHdrGlobalFlags (bit 0x20 = physics; kHdrFlagFixedStaging)
        uint8_t  _pad14[kOffHdrSeqCount - (kOffHdrGlobalFlags + sizeof(uint32_t))];
        uint32_t seqCount;          // kOffHdrSeqCount
        void*    seqPtr;            // kOffHdrSeqPtr -> M2SequenceRec records (stride kSeqStride)
        uint8_t  _pad24[kOffHdrBoneCount - (kOffHdrSeqPtr + sizeof(void*))];
        uint32_t boneCount;         // kOffHdrBoneCount
        void*    boneArray;         // kOffHdrBoneArray -> M2Bone records (post-fixup data ptr)
        uint8_t  _pad34[kOffHdrAttachCount - (kOffHdrBoneArray + sizeof(void*))];
        uint32_t attachCount;       // kOffHdrAttachCount
        void*    attachPtr;         // kOffHdrAttachPtr -> M2Attachment records
        uint32_t boneIdxLutCount;   // kOffHdrBoneIdxLutCount (number of entries in the LUT)
        void*    boneIdxLutPtr;     // kOffHdrBoneIdxLutPtr -> uint16 array indexed by key_bone_id
    };
    static_assert(offsetof(M2FileHeader, globalFlags)    == kOffHdrGlobalFlags,     "M2FileHeader.globalFlags");
    static_assert(offsetof(M2FileHeader, seqCount)       == kOffHdrSeqCount,        "M2FileHeader.seqCount");
    static_assert(offsetof(M2FileHeader, seqPtr)         == kOffHdrSeqPtr,          "M2FileHeader.seqPtr");
    static_assert(offsetof(M2FileHeader, boneCount)      == kOffHdrBoneCount,       "M2FileHeader.boneCount");
    static_assert(offsetof(M2FileHeader, boneArray)      == kOffHdrBoneArray,       "M2FileHeader.boneArray");
    static_assert(offsetof(M2FileHeader, attachCount)    == kOffHdrAttachCount,     "M2FileHeader.attachCount");
    static_assert(offsetof(M2FileHeader, attachPtr)      == kOffHdrAttachPtr,       "M2FileHeader.attachPtr");
    static_assert(offsetof(M2FileHeader, boneIdxLutCount)== kOffHdrBoneIdxLutCount, "M2FileHeader.boneIdxLutCount");
    static_assert(offsetof(M2FileHeader, boneIdxLutPtr)  == kOffHdrBoneIdxLutPtr,   "M2FileHeader.boneIdxLutPtr");

    /** @brief In-model track head: interpolation, global-sequence link, and the per-sequence outer
     *         arrays (timestamps + values). outerCount == 0 means the track carries no data at all. */
    struct M2TrackHead
    {
        uint16_t interp;           // interpolation type
        uint16_t globalSeq;        // global-sequence index (0xFFFF = sequence-driven)
        uint32_t outerCount;       // kOffTrackTimestampsCount relative to the head
        void*    outerPtr;         // kOffTrackTimestampsPtr
        uint32_t valuesOuterCount; // kOffTrackValuesCount
        void*    valuesOuterPtr;   // kOffTrackValuesPtr
    };
    static_assert(sizeof(M2TrackHead) == 0x14, "M2TrackHead size");

    /** @brief Bone record in the header bone array (stride kBoneStride). */
    struct M2Bone
    {
        int32_t     keyBoneId;     // kOffBoneKeyId (canonical slot id; negative = no key bone)
        uint32_t    flags;         // kOffBoneFlags
        int16_t     parent;        // kOffBoneParent (0xFFFF = root)
        uint8_t     _pad0a[kOffBoneNameCrc - (kOffBoneParent + sizeof(int16_t))];
        uint32_t    nameCrc;       // kOffBoneNameCrc (CRC32 of the bone name, for name-based remap)
        M2TrackHead transTrack;    // kOffBoneTransTrack
        M2TrackHead rotTrack;      // kOffBoneRotTrack
        M2TrackHead scaleTrack;    // kOffBoneScaleTrack
        float       pivot[3];      // kOffBonePivot (bone origin in bind space)
    };
    static_assert(offsetof(M2Bone, keyBoneId)  == kOffBoneKeyId,      "M2Bone.keyBoneId");
    static_assert(offsetof(M2Bone, flags)      == kOffBoneFlags,      "M2Bone.flags");
    static_assert(offsetof(M2Bone, parent)     == kOffBoneParent,     "M2Bone.parent");
    static_assert(offsetof(M2Bone, nameCrc)    == kOffBoneNameCrc,    "M2Bone.nameCrc");
    static_assert(offsetof(M2Bone, transTrack) == kOffBoneTransTrack, "M2Bone.transTrack");
    static_assert(offsetof(M2Bone, rotTrack)   == kOffBoneRotTrack,   "M2Bone.rotTrack");
    static_assert(offsetof(M2Bone, scaleTrack) == kOffBoneScaleTrack, "M2Bone.scaleTrack");
    static_assert(offsetof(M2Bone, pivot)      == kOffBonePivot,      "M2Bone.pivot");
    static_assert(sizeof(M2Bone) == kBoneStride, "M2Bone size");

    /** @brief Attachment record (stride kAttachStride): the palette slot and offset a slot-attached
     *         child instance rides, plus the enable track sampled during full builds. */
    struct M2Attachment
    {
        uint32_t    id;
        uint16_t    bone;          // kOffAttachBone
        uint16_t    _pad06;
        float       pos[3];        // kOffAttachPos
        M2TrackHead enableTrack;
    };
    static_assert(offsetof(M2Attachment, bone) == kOffAttachBone, "M2Attachment.bone");
    static_assert(offsetof(M2Attachment, pos)  == kOffAttachPos,  "M2Attachment.pos");
    static_assert(sizeof(M2Attachment) == kAttachStride, "M2Attachment size");

    /** @brief Sequence record view (stride kSeqStride): the flags word the cadence scan reads. */
    struct M2SequenceRec
    {
        uint8_t  _pad00[kOffSeqFlags];
        uint32_t flags;            // kOffSeqFlags (bit 0x1 = plays once, then holds)
    };
    static_assert(offsetof(M2SequenceRec, flags) == kOffSeqFlags, "M2SequenceRec.flags");

    /** @brief Track object read by the evaluators: the timestamp and value sub-arrays (count + ptr each). */
    struct M2Track
    {
        uint8_t   _pad00[kOffTrackTimestampsCount];
        uint32_t  timestampsCount; // kOffTrackTimestampsCount
        void*     timestampsPtr;   // kOffTrackTimestampsPtr
        uint32_t  valuesCount;     // kOffTrackValuesCount
        void*     valuesPtr;       // kOffTrackValuesPtr
    };
    static_assert(offsetof(M2Track, timestampsCount) == kOffTrackTimestampsCount, "M2Track.timestampsCount");
    static_assert(offsetof(M2Track, timestampsPtr)   == kOffTrackTimestampsPtr,   "M2Track.timestampsPtr");
    static_assert(offsetof(M2Track, valuesCount)     == kOffTrackValuesCount,     "M2Track.valuesCount");
    static_assert(offsetof(M2Track, valuesPtr)       == kOffTrackValuesPtr,       "M2Track.valuesPtr");

    /** @brief Runtime bone state (stride kRuntimeBoneStride): the persisted sampling records the
     *         recomposition walk consumes, and the sequence/blend anchors the cadence scan reads. */
    struct RuntimeBone
    {
        uint32_t transHint[2];     // key-search hints (channel A/B)
        float    transValue[3];    // kOffRtBoneTransValue: last sampled translation
        uint32_t rotHint[2];
        float    quatValue[4];     // kOffRtBoneQuatValue: last sampled rotation
        uint32_t scaleHint[2];
        float    scaleValue[3];    // kOffRtBoneScaleValue: last sampled scale
        uint32_t time;             // current channel-A time within the sequence (ms)
        uint16_t animIndex;        // kOffRuntimeBoneAnimIdx: per-animation inner-slot selector
        uint16_t boneIndex;
        uint16_t assignedSeq;      // kOffRtBoneAssignedSeq (0xFFFF = inherit from the parent bone)
        uint8_t  clampDone;
        uint8_t  callbackFlag;
        int32_t  seqStart;         // kOffRtBoneSeqStart (absolute clock ms)
        int32_t  seqEnd;           // kOffRtBoneSeqEnd (absolute clock ms)
        float    timeScale;        // kOffRtBoneTimeScale
        float    invSpeed;
        int32_t  timeOffset;       // kOffRtBoneTimeOffset
        int32_t  repeatCount;
        uint32_t timeB;            // channel-B (blend-from) mirror of +0x40..+0x5C
        uint16_t animIndexB;
        uint8_t  _pad6a[kOffRtBoneBlendSeq - 0x6A];
        uint16_t blendSeq;         // kOffRtBoneBlendSeq (0xFFFF = no blend running)
        uint8_t  _pad6e[kOffRtBoneProcMatrix - (kOffRtBoneBlendSeq + sizeof(uint16_t))];
        float*   procMatrix;       // kOffRtBoneProcMatrix (externally driven 4x4; null = none)
        uint32_t flagMask;         // kOffRtBoneFlagMask (OR'd into the static bone flags)
        uint32_t pendingSeq;       // kOffRtBonePendingSeq (0xFFFFFFFF idle)
        uint8_t  _pad94[kOffRtBoneBlendWeight - 0x94];
        float    blendWeight;      // kOffRtBoneBlendWeight (0 = no blend contribution)
    };
    static_assert(offsetof(RuntimeBone, transValue)  == kOffRtBoneTransValue,  "RuntimeBone.transValue");
    static_assert(offsetof(RuntimeBone, quatValue)   == kOffRtBoneQuatValue,   "RuntimeBone.quatValue");
    static_assert(offsetof(RuntimeBone, scaleValue)  == kOffRtBoneScaleValue,  "RuntimeBone.scaleValue");
    static_assert(offsetof(RuntimeBone, animIndex)   == kOffRuntimeBoneAnimIdx,"RuntimeBone.animIndex");
    static_assert(offsetof(RuntimeBone, assignedSeq) == kOffRtBoneAssignedSeq, "RuntimeBone.assignedSeq");
    static_assert(offsetof(RuntimeBone, seqStart)    == kOffRtBoneSeqStart,    "RuntimeBone.seqStart");
    static_assert(offsetof(RuntimeBone, seqEnd)      == kOffRtBoneSeqEnd,      "RuntimeBone.seqEnd");
    static_assert(offsetof(RuntimeBone, timeScale)   == kOffRtBoneTimeScale,   "RuntimeBone.timeScale");
    static_assert(offsetof(RuntimeBone, timeOffset)  == kOffRtBoneTimeOffset,  "RuntimeBone.timeOffset");
    static_assert(offsetof(RuntimeBone, blendSeq)    == kOffRtBoneBlendSeq,    "RuntimeBone.blendSeq");
    static_assert(offsetof(RuntimeBone, procMatrix)  == kOffRtBoneProcMatrix,  "RuntimeBone.procMatrix");
    static_assert(offsetof(RuntimeBone, flagMask)    == kOffRtBoneFlagMask,    "RuntimeBone.flagMask");
    static_assert(offsetof(RuntimeBone, pendingSeq)  == kOffRtBonePendingSeq,  "RuntimeBone.pendingSeq");
    static_assert(offsetof(RuntimeBone, blendWeight) == kOffRtBoneBlendWeight, "RuntimeBone.blendWeight");
    static_assert(sizeof(RuntimeBone) == kRuntimeBoneStride, "RuntimeBone size");

    /** @brief Ribbon emitter: the draw-loop layer count and the per-layer texture-handle array pointer. */
    struct RibbonEmitter
    {
        uint8_t  _pad00[kOffRibbonLayerCount];
        uint32_t layerCount;       // kOffRibbonLayerCount (draw-loop bound)
        uint8_t  _pad11c[kOffRibbonTexHandlePtr - (kOffRibbonLayerCount + sizeof(uint32_t))];
        void**   texHandles;       // kOffRibbonTexHandlePtr -> per-layer texture-handle array (stride 4)
    };
    static_assert(offsetof(RibbonEmitter, layerCount) == kOffRibbonLayerCount,   "RibbonEmitter.layerCount");
    static_assert(offsetof(RibbonEmitter, texHandles) == kOffRibbonTexHandlePtr, "RibbonEmitter.texHandles");

    /** @brief Character model object: race/gender ids and the root scene node pointer. */
    struct CharModelObject
    {
        uint8_t  _pad00[kOffCmoRace];
        uint32_t raceId;           // kOffCmoRace
        uint32_t genderId;         // kOffCmoGender (0 = male, 1 = female)
        uint8_t  _pad20[kOffCmoSceneNode - (kOffCmoGender + sizeof(uint32_t))];
        void*    sceneNode;        // kOffCmoSceneNode -> SceneNode
    };
    static_assert(offsetof(CharModelObject, raceId)    == kOffCmoRace,      "CharModelObject.raceId");
    static_assert(offsetof(CharModelObject, genderId)  == kOffCmoGender,    "CharModelObject.genderId");
    static_assert(offsetof(CharModelObject, sceneNode) == kOffCmoSceneNode, "CharModelObject.sceneNode");

    /** @brief Scene node: the CharModelObject that owns this node. */
    struct SceneNode
    {
        uint8_t  _pad00[kOffSceneNodeOwner];
        void*    owner;            // kOffSceneNodeOwner -> CharModelObject
    };
    static_assert(offsetof(SceneNode, owner) == kOffSceneNodeOwner, "SceneNode.owner");
#pragma pack(pop)

    // --- signatures ---
    // Model init / skin finalize: native this-in-ECX; declared with a dummy second parameter so the
    // trampoline routes the model into the this-register.
    using M2_InitFn         = int(__fastcall*)(void* model);
    using M2_FinalizeSkinFn = void(__fastcall*)(void* model);
    // Anim read-completion callback (node on stack).
    using M2_AnimLoadCompleteFn = void(__cdecl*)(void* node);
    // Per-batch alpha setter: native this-in-ECX.
    using M2_SetupBatchAlphaFn = void(__fastcall*)(void* drawContext);
    using M2_SortOpaqueGeoBatchesFn = int(__cdecl*)(void* lhs, void* rhs);
    // Alpha-test reference push (ref on stack).
    using M2_PushAlphaRefFn = void(__cdecl*)(float ref);
    // Ribbon de-relocator (base, fileSize, ctx, ribbons): rebases each ribbon's sub-array offsets.
    using M2_RibbonDeRelocateFn = int(__cdecl*)(int base, unsigned int fileSize, int ctx, unsigned int* ribbons);
    // Ribbon emitter draw: native this-in-ECX.
    using M2_RibbonDrawFn = int(__fastcall*)(void* emitter, void* edx, void* stateBlock);
    // Texture-handle resolver (handle, ...).
    using M2_TexResolveFn = void*(__cdecl*)(void* handle, int a, int b);
    // Sampler bind: native this-in-ECX.
    using M2_SamplerBindFn = void(__fastcall*)(void* device, void* edx, uint32_t selector, void* tex);

    // --- attachment / resource signatures ---
    // GetRenderCtx(cmo, edx, keyBuf, 0): ret 8 (2 stack args: keyBuf + trailing zero).
    using M2_GetRenderCtxFn     = void*(__fastcall*)(void* cmo, void* edx, void* keyBuf, uint32_t zero);
    // AttachToScene(renderCtx, edx, subObj, slot, 0, 0): ret 16 (4 stack args: subObj, slot, 0, 0).
    using M2_AttachToSceneFn    = void (__fastcall*)(void* renderCtx, void* edx, void* subObj, uint32_t slot, uint32_t zero1, uint32_t zero2);
    // DetachSlot(subObj, edx, slot): detaches the M2 from a scene slot, releasing its render ctx.
    using M2_DetachSlotFn       = void (__fastcall*)(void* subObj, void* edx, uint32_t slot);
    // ReleaseRenderCtx(renderCtx, edx): releases a render context.
    using M2_ReleaseRenderCtxFn = void (__fastcall*)(void* renderCtx, void* edx);
    // BindTexSlot(renderCtx, edx, key, modelPtr): ret 8 (key=2, then modelPtr on stack).
    using M2_BindTexSlotFn      = void (__fastcall*)(void* renderCtx, void* edx, uint32_t key, void* modelPtr);
    // LoadResource(path, flags, statusOut, flags2): same call shape as Gx::TextureCreate.
    using M2_LoadResourceFn     = void*(__cdecl*)(const char* path, uint32_t flags, int* statusOut, uint32_t flags2);
    // ReleaseResource(resource): releases a resource handle returned by LoadResource.
    using M2_ReleaseResourceFn  = void (__cdecl*)(void* resource);

    // --- per-frame / slot hook signatures ---
    // PerFrameUpdate(renderCtx, edx): per-render-ctx per-frame scene-graph update.
    using M2_PerFrameUpdateFn   = void (__fastcall*)(void* renderCtx, void* edx);
    // BuildBonePalette(instance, edx, sa1, sa2, sa3, sa4, sa5): fills the instance bone palette
    // from the current animation pose. fastcall, 5 stack args, ret 0x14. Hook POST-order to
    // override the engine's fill (e.g. CharSweep for collection M2s attached to a character).
    using M2_BuildBonePaletteFn = void (__fastcall*)(void* renderCtx, void* edx,
        void* sa1, void* sa2, void* sa3, uint32_t sa4, uint32_t sa5);
    using M2_RenderBatchShadowMapFn = void (__fastcall*)(
        void* instance, void* edx, uint32_t batchMode, void* skinBatch, void* drawList,
        uint32_t drawIndex, void* skinSection, void* previousSection);
    // SlotDispatch(cmo, edx, modelSlot, itemDataPtr, postFlag): equip-slot handler; loads the model.
    using M2_SlotDispatchFn     = void (__fastcall*)(void* cmo, void* edx, uint32_t modelSlot, void* itemDataPtr, uint32_t postFlag);
    // SlotClear(cmo, edx, equipSlotWow): clears a WoW equipment slot on the CMO.
    using M2_SlotClearFn        = void (__fastcall*)(void* cmo, void* edx, uint32_t equipSlotWow);
}
