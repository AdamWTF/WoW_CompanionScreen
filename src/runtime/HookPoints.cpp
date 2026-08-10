// Named hook points: the addresses behind WXL_Api::HookAttachByName, so an extension can attach to
// one without including an offsets/ header itself.
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

#include "runtime/HookPoints.hpp"

#include "common/Log.hpp"
#include "engine/hook/Hook.hpp"

#include "offsets/engine/Boot.hpp"
#include "offsets/engine/Camera.hpp"
#include "offsets/engine/Frame.hpp"
#include "offsets/engine/Gx.hpp"
#include "offsets/engine/Io.hpp"
#include "offsets/engine/Liquid.hpp"
#include "offsets/engine/Lua.hpp"
#include "offsets/engine/Mem.hpp"
#include "offsets/engine/Shader.hpp"
#include "offsets/engine/Sky.hpp"
#include "offsets/engine/Sound.hpp"
#include "offsets/game/ADT.hpp"
#include "offsets/game/DB2.hpp"
#include "offsets/game/Doodad.hpp"
#include "offsets/game/GroundEffect.hpp"
#include "offsets/game/M2.hpp"
#include "offsets/game/Unit.hpp"
#include "offsets/game/WMO.hpp"
#include "offsets/game/World.hpp"
#include "offsets/game/WorldScene.hpp"

#include <cstring>

namespace wxl::runtime::hookpoints
{
    namespace
    {
        namespace adt    = wxl::offsets::game::adt;
        namespace boot   = wxl::offsets::engine::boot;
        namespace cam    = wxl::offsets::engine::camera;
        namespace db2    = wxl::offsets::game::db2;
        namespace dd     = wxl::offsets::game::doodad;
        namespace frm    = wxl::offsets::engine::frame;
        namespace grass  = wxl::offsets::game::groundeffect;
        namespace gxoff  = wxl::offsets::engine::gx;
        namespace io     = wxl::offsets::engine::io;
        namespace liq    = wxl::offsets::engine::liquid;
        namespace lua    = wxl::offsets::engine::lua;
        namespace mem    = wxl::offsets::engine::mem;
        namespace m2     = wxl::offsets::game::m2;
        namespace shoff  = wxl::offsets::engine::shader;
        namespace sky    = wxl::offsets::engine::sky;
        namespace snd    = wxl::offsets::engine::sound;
        namespace unit   = wxl::offsets::game::unit;
        namespace wld    = wxl::offsets::game::world;
        namespace wmo    = wxl::offsets::game::wmo;
        namespace wscene = wxl::offsets::game::worldscene;

        struct Point
        {
            const char* name;
            uintptr_t   address;
        };

        // Every hook point an extension may attach to by name. Add an entry here (and give it a
        // stable name) rather than have the extension include an offsets/ header of its own.
        //
        // A name is part of the ABI: it may be added, never renamed or dropped. Two names may resolve
        // to one address -- the chain is keyed by the address, so both detours land in the same chain.
        // Only function entries belong here; the patch sites, return-address anchors and data globals
        // that also live in offsets/ are not detour targets and are deliberately absent.
        constexpr Point kPoints[] = {
            // --- startup / per-frame anchors -------------------------------------------------------
            { "Boot.EngineInit",              boot::kEngineInit },
            { "Frame.Pump",                   frm::kFramePump },
            { "Camera.BuildMatrices",         cam::kBuildCameraMatrices },
            { "Camera.GetActive",             cam::kGetActiveCamera },

            // --- archive / file I/O ----------------------------------------------------------------
            { "Io.FileOpen",                  io::kFileOpen },
            { "Io.FileOpenAlt",               io::kFileOpen2 },
            { "Io.FileSize",                  io::kFileSize },
            { "Io.FileRead",                  io::kFileRead },
            { "Io.FileSeek",                  io::kFileSeek },
            { "Io.FileClose",                 io::kFileClose },
            { "Io.FileExists",                wld::kFileExists },
            { "Io.ArchiveMount",              io::kArchiveMount },
            { "Io.ArchiveOpen",               io::kMopaqOpenArchive },
            { "Io.InitializeWowConfig",       io::kInitializeWowConfig },

            // --- script surface --------------------------------------------------------------------
            { "Lua.GetContext",               lua::kFrameScriptGetContext },
            { "Lua.RegisterFunction",         lua::kFrameScriptRegisterFunction },
            { "Lua.Execute",                  lua::kFrameScriptExecute },
            { "Lua.FillScriptMethodTable",    lua::kFillScriptMethodTable },
            { "Lua.ValidateFunctionPointer",  lua::kValidateFunctionPointer },
            { "Lua.GetObjectThis",            lua::kGetObjectThis },

            // --- allocator / caches ----------------------------------------------------------------
            { "Mem.Alloc",                    mem::kAlloc },
            { "Mem.Free",                     mem::kFree },
            { "Mem.M2CacheGarbageCollect",    mem::kM2CacheGarbageCollect },
            { "Mem.M2SceneAdvanceTime",       mem::kM2SceneAdvanceTime },
            { "Mem.TextureCacheUpdate",       mem::kTextureCacheUpdate },

            // --- graphics device / textures --------------------------------------------------------
            { "Gx.SetDefaultWindow",          gxoff::kDeviceSetDefWindow },
            { "Gx.SceneClear",                gxoff::kGxSceneClear },
            { "Gx.ShaderUpdateProjMatrix",    gxoff::kShaderUpdateProjMatrix },
            { "Gx.WorldOnRender",             gxoff::kWorldOnRender },
            { "Gx.WorldRenderFinalize",       gxoff::kWorldRenderFinalize },
            { "Gx.GlueModelRender",           gxoff::kSimpleModelFFXRender },
            { "Gx.GlueModelRegisterMethods",  gxoff::kSimpleModelRegisterMethods },
            { "Gx.TextureCreate",             gxoff::kTextureCreate },
            { "Gx.TextureUpdate",             gxoff::kTextureUpdate },
            { "Gx.TextureResolve",            adt::kTexResolve },
            { "Gx.TextureRelease",            adt::kTextureRelease },
            { "Gx.TexSetWrap",                gxoff::kGxTexSetWrap },
            { "Gx.StateSet",                  shoff::kGxStateSet },
            { "Gx.StateDirty",                shoff::kGxStateDirty },
            { "Gx.RenderStateSet",            adt::kGxRsSetInt },
            { "Gx.LiquidRenderPass",          gxoff::kLiquidRenderPass },

            // --- programmable shader path ----------------------------------------------------------
            { "Shader.EffectActivate",        shoff::kEffectActivate },
            { "Shader.EffectBind",            shoff::kEffectBind },
            { "Shader.OutdoorIndexDriver",    shoff::kOutdoorIndexDriver },
            { "Shader.PixelIndexDriver",      shoff::kPixelIndexDriver },
            { "Shader.NativeBlsLoad",         shoff::kNativeBlsLoad },
            { "Shader.CreateVertex",          shoff::kShaderCreateVertex },
            { "Shader.ConstantsSet",          shoff::kShaderConstantsSet },
            { "Shader.TerrainPermutationIndex", shoff::kTerrainShaderPermutationIndex },
            { "Shader.TerrainSelect",         shoff::kTerrainShaderSelect },
            { "Shader.ShadowTierGet",         adt::kShadowTierGetter },

            // --- sky / day-night -------------------------------------------------------------------
            { "Sky.CloudsGenerate",           sky::kCloudsGenerate },
            { "Sky.DayNightGetInfo",          sky::kDayNightGetInfo },
            { "Sky.SetOverrideFog",           sky::kSetOverrideFog },
            { "Sky.ClearOverrideFog",         sky::kClearOverrideFog },

            // --- sound -----------------------------------------------------------------------------
            { "Sound.SetMasterVolume",        snd::kSetMasterVolume },
            { "Sound.PlaySound",              snd::kPlaySound },
            { "Sound.PlaySoundKit",           snd::kPlaySoundKit },

            // --- liquid ----------------------------------------------------------------------------
            { "Liquid.MaterialWaterRender",   liq::kMaterialWaterRender },
            { "Liquid.MaterialWaterNoSpecRender", liq::kMaterialWaterNoSpecRender },
            { "Liquid.MaterialBankGetMaterial", liq::kMaterialBankGetMaterial },
            { "Liquid.ChunkGeomGetBuffers",   liq::kChunkGeomGetBuffers },
            { "Liquid.MeshGeomGetBuffers",    liq::kMeshGeomGetBuffers },
            { "Liquid.MapChunkBufAlloc",      liq::kMapChunkBufAlloc },
            { "Liquid.ChunkVertexEmit",       liq::kChunkVertexEmit },
            { "Liquid.ConstantUpload",        liq::kConstantUpload },
            { "Liquid.SettingsAnimTexture",   liq::kSettingsAnimTexture },
            { "Liquid.GxPoolCreate",          liq::kGxPoolCreate },
            { "Liquid.GxBufCreate",           liq::kGxBufCreate },
            { "Liquid.GxBufSizeSet",          liq::kGxBufSizeSet },
            { "Liquid.MapGetHeightTerrain",   liq::kMapGetHeightTerrain },

            // --- world tick / streaming / picking ---------------------------------------------------
            { "World.Tick",                   wld::kTick },
            { "World.Enter",                  wld::kEnter },
            { "World.MapEnter",               wld::kMapEnter },
            { "World.PrepareUpdate",          wld::kPrepareUpdate },
            { "World.Render",                 wld::kRender },
            { "World.StreamingTick",          wld::kStreamingTick },
            { "World.PurgeAllTiles",          wld::kPurgeAllTiles },
            { "World.TileFactory",            wld::kTileFactory },
            { "World.TileLoader",             wld::kTileLoader },
            { "World.TileUnload",             wld::kTileUnload },
            { "World.TileDestroy",            wld::kTileDestroy },
            { "World.SetFarClip",             wld::kSetFarClip },
            { "World.ValidateFarClip",        wld::kValidateFarClip },
            { "World.GetScreenCoordinates",   wld::kGetScreenCoordinates },
            { "World.PickAtScreen",           wld::kPickAtScreen },
            { "World.ScreenToRay",            wld::kScreenToRay },
            { "World.IntersectWrapper",       wld::kIntersectWrapper },
            { "World.Intersect",              wld::kWorldIntersect },
            { "World.AsyncFileReadInitialize", wld::kAsyncFileReadInitialize },
            { "World.AsyncFileReadObject",     wld::kAsyncFileReadObject },
            { "World.AsyncFileReadAllocObject", wld::kAsyncFileReadAllocObject },
            { "World.AsyncFileReadLinkObject", wld::kAsyncFileReadLinkObject },
            { "World.AsyncFileReadWait",      wld::kAsyncFileReadWait },
            { "World.AsyncWaitAll",           wld::kAsyncWaitAll },
            { "World.AsyncPending",           wld::kAsyncPending },
            { "World.AsyncServiceQueues",     wld::kAsyncServiceQueues },
            { "World.AsyncDestroy",           wld::kAsyncDestroy },
            { "World.AsyncQueueAlloc",        wld::kAsyncQueueAlloc },
            { "World.AsyncThreadWrap",        wld::kAsyncThreadWrap },

            // --- terrain ---------------------------------------------------------------------------
            { "Adt.GetChunk",                 adt::kGetChunk },
            { "Adt.TileAreaLoad",             adt::kTileAreaLoad },
            { "Adt.TileAreaCreate",           adt::kTileAreaCreate },
            { "Adt.TileAreaAsyncLoadCallback", adt::kTileAreaAsyncLoadCallback },
            { "Adt.TileAreaDestroy",          adt::kTileAreaDestroy },
            { "Adt.PrepareChunk",             adt::kPrepareChunk },
            { "Adt.ProcessIffChunks",         adt::kChunkProcessIffChunks },
            { "Adt.ChunkBuild",               adt::kChunkBuild },
            { "Adt.ChunkFrustumCull",         adt::kChunkFrustumCull },
            { "Adt.NearObjectCount",          adt::kNearObjectCount },
            { "Adt.AreaUpdate",               adt::kAreaUpdate },
            { "Adt.AllocRawAreaData",         adt::kAllocRawAreaData },
            { "Adt.FreeRawAreaData",          adt::kFreeRawAreaData },
            { "Adt.LoadWdl",                  adt::kLoadWdl },
            { "Adt.AllocAreaLow",             adt::kAllocAreaLow },
            { "Adt.FreeAreaLow",              adt::kFreeAreaLow },
            { "Adt.AreaLoadTextures",         adt::kAreaLoadTextures },
            { "Adt.LoadTerrainTexture",       adt::kLazyLoadTexSlot },
            { "Adt.MapLoadTexture",           adt::kMapLoadTexture },
            { "Adt.AllocTerrainTexture",      adt::kAllocTerrainTexture },
            { "Adt.BuildLayerAlpha",          adt::kBuildLayerAlpha },
            { "Adt.SurfaceChunkDraw",         adt::kSurfaceChunkDraw },
            { "Adt.SurfaceChunkDrawShader",   adt::kSurfaceChunkDrawShader },
            { "Adt.BuildTerrainConstants",    adt::kBuildTerrainConstants },

            // --- placed doodads / grass -------------------------------------------------------------
            { "Doodad.SpawnFromMddf",         dd::kSpawnFromMDDF },
            { "Doodad.Purge",                 dd::kDoodadPurge },
            { "Grass.ChunkConstantUpload",    grass::kChunkConstantUpload },
            { "Grass.InitShaderConstants",    grass::kInitShaderConstants },

            // --- objects / units --------------------------------------------------------------------
            { "Unit.GetObjectByGuid",         unit::kGetObjectByGuid },
            { "Unit.EnumObjects",             unit::kEnumObjects },
            { "Unit.GetActivePlayerGuid",     unit::kActivePlayerGuid },
            { "Unit.Reaction",                unit::kUnitReaction },
            { "Unit.ObjectUpdate",            unit::kObjectUpdateHandler },
            { "Unit.ObjectDestroy",           unit::kObjectDestroyHandler },
            { "Unit.TargetSet",               unit::kTargetSet },

            // --- client data tables ------------------------------------------------------------------
            { "Db2.MapLoad",                  db2::mapdef::kLoader },
            { "Db2.MapPostLoadBuild",         db2::mapdef::kPostLoadBuild },
            { "Db2.CharSectionsLookup",       db2::charsectionsdef::kRecordLookup },
            { "Db2.CharSectionsCacheBuild",   db2::charsectionsdef::kCacheBuilder },
            { "Db2.ItemDisplayInfoLookup",    db2::itemdisplayinfo::kLookup },

            // --- M2: load / parse --------------------------------------------------------------------
            { "M2.Init",                      m2::kInit },
            { "M2.SharedInitialize",          m2::kSharedInitialize },
            { "M2.InitializeLoaded",          m2::kInitializeLoaded },
            { "M2.ReadVertices",              m2::kReadVertices },
            { "M2.ReadByteArray",             m2::kReadByteArray },
            { "M2.ReadVector3",               m2::kReadVector3 },
            { "M2.ReadInt32Array",            m2::kReadInt32Array },
            { "M2.ReadInt16Array",            m2::kReadInt16Array },
            { "M2.ReadAnimations",            m2::kReadAnimations },
            { "M2.ReadTextures",              m2::kReadTextures },
            { "M2.ReadEvents",                m2::kReadEvents },
            { "M2.ReadColors",                m2::kReadColors },
            { "M2.ReadTransparency",          m2::kReadTransparency },
            { "M2.ReadBones",                 m2::kReadBones },
            { "M2.ReadUVAnimation",           m2::kReadUVAnimation },
            { "M2.ReadAttachments",           m2::kReadAttachments },
            { "M2.ReadLights",                m2::kReadLights },
            { "M2.ReadCameras",               m2::kReadCameras },
            { "M2.ReadParticleEmitters",      m2::kReadParticleEmitters },
            { "M2.RibbonDeRelocate",          m2::kRibbonDeRelocate },
            { "M2.BuildSkinPath",             m2::kBuildSkinPath },
            { "M2.LoadSkinProfile",           m2::kLoadSkinProfile },
            { "M2.FinishLoadingSkinProfile",  m2::kFinishLoadingSkinProfile },
            { "M2.FinalizeSkin",              m2::kFinalizeSkin },
            { "M2.BuildBatchMaterial",        m2::kBuildBatchMaterial },
            { "M2.BuildAnimPath",             m2::kBuildAnimPath },
            { "M2.SequenceLoad",              m2::kSequenceLoad },
            { "M2.PerSeqDeReloc",             m2::kPerSeqDeReloc },
            { "M2.AnimLoadComplete",          m2::kAnimLoadComplete },
            { "M2.BufferAlloc",               m2::kBufferAlloc },
            { "M2.BufferFree",                m2::kBufferFree },

            // --- M2: scene / instance lifetime -------------------------------------------------------
            { "M2.CreateSceneModel",          m2::kCreateSceneModel },
            { "M2.AttachToScene",             m2::kAttachToScene },
            { "M2.DetachSlot",                m2::kDetachSlot },
            { "M2.ReleaseRenderCtx",          m2::kReleaseRenderCtx },
            { "M2.BindTexSlot",               m2::kBindTexSlot },
            { "M2.SetSequenceCallback",       m2::kSetSequenceCallback },
            { "M2.SetEventCallback",          m2::kSetEventCallback },
            { "M2.SetBoneSequence",           m2::kSetBoneSequence },
            { "M2.CharModelSlotDispatch",     m2::kCharModelSlotDispatch },
            { "M2.CharModelSlotClear",        m2::kCharModelSlotClear },
            { "M2.ModelLightingCallback",     m2::kModelLightingCallback },

            // --- M2: animate / render ----------------------------------------------------------------
            { "M2.PerFrameUpdate",            m2::kM2PerFrameUpdate },
            { "M2.CacheBeginThread",          m2::kCacheBeginThread },
            { "M2.CacheWaitThread",           m2::kCacheWaitThread },
            { "M2.TrackEvalVec3",             m2::kTrackEvalVec3 },
            { "M2.TrackEvalQuat",             m2::kTrackEvalQuat },
            { "M2.BuildBonePalette",          m2::kBuildBonePalette },
            { "M2.BuildBonePaletteSimple",    m2::kBuildBonePaletteSimple },
            { "M2.IsDrawable",                m2::kIsDrawable },
            { "M2.IsBatchDoodadCompatible",   m2::kIsBatchDoodadCompatible },
            { "M2.SetupMaterial",             m2::kSetupMaterial },
            { "M2.PushAlphaRef",              m2::kPushAlphaRef },
            { "M2.ShaderConstUnlock",         m2::kShaderConstUnlock },
            { "M2.SortOpaqueGeoBatches",      m2::kSortOpaqueGeoBatches },
            { "M2.DrawBatch",                 gxoff::kDrawTriangleBatch },
            { "M2.DrawBatchDoodad",           gxoff::kDrawBatchDoodad },
            { "M2.ProjectedDecalDraw",        m2::kProjectedDecalDraw },
            { "M2.ShadowMapBatches",          m2::kShadowMapBatches },
            { "M2.RenderBatchShadowMap",      m2::kRenderBatchShadowMap },
            { "M2.RibbonDraw",                m2::kRibbonDraw },
            { "M2.SceneTriangleHitTest",      m2::kSceneTriangleHitTest },

            // --- M2: particles -----------------------------------------------------------------------
            { "M2.AnimateParticlesMT",        m2::kAnimateParticlesMT },
            { "M2.EmitNewParticles",          m2::kEmitNewParticles },
            { "M2.EmitterSync",               m2::kEmitterSync },
            { "M2.EmitterSyncAllocation",     m2::kEmitterSyncAllocation },
            { "M2.ParticleSetZSource",        m2::kSetZsource },

            // --- WMO ----------------------------------------------------------------------------------
            { "Wmo.SpawnFromModf",            wmo::kSpawnFromModf },
            { "Wmo.RootWalk",                 wmo::kRootWalk },
            { "Wmo.RootCreateData",           wmo::kRootCreateData },
            { "Wmo.RootComplete",             wmo::kRootComplete },
            { "Wmo.GroupParse",               wmo::kGroupParse },
            { "Wmo.GroupWalk",                wmo::kGroupWalk },
            { "Wmo.GroupWalkOptional",        wmo::kGroupWalkOptional },
            { "Wmo.GroupComplete",            wmo::kGroupComplete },
            { "Wmo.CreateMaterial",           wmo::kResolveMaterialTexture },
            { "Wmo.CreateMaterials",          wmo::kCreateMaterials },
            { "Wmo.UpdateMaterials",          wmo::kUpdateMaterials },
            { "Wmo.FixColorVertexAlpha",      wmo::kFixColorVertexAlpha },
            { "Wmo.BspInit",                  wmo::kBspInit },
            { "Wmo.BspRaycastRefine",         wmo::kBspRaycastRefine },
            { "Wmo.WaitLoad",                 wmo::kWaitLoad },
            { "Wmo.WaitLoadGroup",            wmo::kWaitLoadGroup },
            { "Wmo.FreeMapObj",               wmo::kFreeMapObj },
            { "Wmo.FreeMapObjGroup",          wmo::kFreeMapObjGroup },
            { "Wmo.CullBatch",                wmo::kCullBatch },
            { "Wmo.CullSortTable",            wmo::kCullSortTable },
            { "Wmo.LocateViewerMapObjs",      wmo::kLocateViewerMapObjs },
            { "Wmo.PortalTraverse",           wmo::kPortalTraverse },
            { "Wmo.PortalVisibility",         wmo::kPortalVisibility },
            { "Wmo.PortalRectAccum",          wmo::kPortalRectAccum },
            { "Wmo.GroupResidentAccessor",    wmo::kGroupResidentAccessor },
            { "Wmo.CameraInGroupTest",        wmo::kCameraInGroupTest },
            { "Wmo.HorizonAabbTest",          wmo::kHorizonAabbTest },
            { "Wmo.CreateOccluders",          wmo::kCreateOccluders },
            { "Wmo.AllocOccluder",            wmo::kAllocOccluder },
            { "Wmo.AddOccluderEdge",          wmo::kAddOccluderEdge },
            { "Wmo.ExtRender",                wmo::kExtRender },
            { "Wmo.IntRender",                wmo::kIntRender },
            { "Wmo.CompositeEffectBind",      shoff::kEffectBind },

            // --- world scene cull ---------------------------------------------------------------------
            { "Scene.CullMapObjDefGroupFromExterior", wscene::kCullMapObjDefGroupFromExterior },
            { "Scene.ClipBufferStampPolyline", wscene::kClipBufferStampPolyline },
        };

        const Point* Find(const char* name)
        {
            if (!name) return nullptr;
            for (const Point& p : kPoints)
                if (std::strcmp(p.name, name) == 0) return &p;
            return nullptr;
        }
    }

    int AttachByName(const char* pointName, void* detour, void** original, int priority)
    {
        const Point* p = Find(pointName);
        if (!p)
        {
            WLOG_ERROR("hookpoints: '%s' is not a registered hook point", pointName ? pointName : "(null)");
            return 0;
        }
        return wxl::hook::Install(pointName, p->address, detour, original, priority) ? 1 : 0;
    }
}
