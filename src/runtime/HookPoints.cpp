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

#include "offsets/engine/Gx.hpp"
#include "offsets/engine/Mem.hpp"
#include "offsets/engine/Shader.hpp"
#include "offsets/game/ADT.hpp"
#include "offsets/game/M2.hpp"
#include "offsets/game/WMO.hpp"
#include "offsets/game/World.hpp"

#include <cstring>

namespace wxl::runtime::hookpoints
{
    namespace
    {
        namespace adt   = wxl::offsets::game::adt;
        namespace gxoff = wxl::offsets::engine::gx;
        namespace mem   = wxl::offsets::engine::mem;
        namespace m2    = wxl::offsets::game::m2;
        namespace shoff = wxl::offsets::engine::shader;
        namespace wld   = wxl::offsets::game::world;
        namespace wmo   = wxl::offsets::game::wmo;

        struct Point
        {
            const char* name;
            uintptr_t   address;
        };

        // Every hook point an extension may attach to by name. Add an entry here (and give it a
        // stable name) rather than have the extension include an offsets/ header of its own.
        constexpr Point kPoints[] = {
            { "M2.BuildBonePalette",          m2::kBuildBonePalette },
            { "M2.CacheBeginThread",          m2::kCacheBeginThread },
            { "M2.IsDrawable",                m2::kIsDrawable },
            { "M2.IsBatchDoodadCompatible",   m2::kIsBatchDoodadCompatible },
            { "M2.SetupMaterial",             m2::kSetupMaterial },
            { "M2.PerFrameUpdate",            m2::kM2PerFrameUpdate },
            { "M2.Init",                      m2::kInit },
            { "M2.FinalizeSkin",              m2::kFinalizeSkin },
            { "M2.BuildBatchMaterial",        m2::kBuildBatchMaterial },
            { "M2.AnimLoadComplete",          m2::kAnimLoadComplete },
            { "M2.DrawBatch",                 gxoff::kDrawTriangleBatch },
            { "M2.RibbonDraw",                m2::kRibbonDraw },
            { "M2.SceneTriangleHitTest",      m2::kSceneTriangleHitTest },
            { "M2.SortOpaqueGeoBatches",      m2::kSortOpaqueGeoBatches },
            { "M2.InitializeLoaded",          m2::kInitializeLoaded },
            { "M2.RenderBatchShadowMap",      m2::kRenderBatchShadowMap },
            { "M2.BufferAlloc",               m2::kBufferAlloc },
            { "M2.BufferFree",                m2::kBufferFree },
            { "World.AsyncFileReadInitialize", wld::kAsyncFileReadInitialize },
            { "World.AsyncFileReadObject",     wld::kAsyncFileReadObject },
            { "World.ValidateFarClip",         wld::kValidateFarClip },
            { "Mem.Alloc",                     mem::kAlloc },
            { "Mem.M2SceneAdvanceTime",        mem::kM2SceneAdvanceTime },
            { "Adt.TileAreaLoad",              adt::kTileAreaLoad },
            { "Adt.ProcessIffChunks",          adt::kChunkProcessIffChunks },
            { "Adt.TileAreaDestroy",           adt::kTileAreaDestroy },
            { "Adt.LoadWdl",                   adt::kLoadWdl },
            { "Adt.AreaLoadTextures",          adt::kAreaLoadTextures },
            { "Adt.LoadTerrainTexture",        adt::kLazyLoadTexSlot },
            { "Adt.ChunkBuild",                adt::kChunkBuild },
            { "Adt.SurfaceChunkDrawShader",    adt::kSurfaceChunkDrawShader },
            { "Adt.BuildTerrainConstants",     adt::kBuildTerrainConstants },
            { "Adt.AreaUpdate",                adt::kAreaUpdate },
            { "Wmo.SpawnFromModf",             wmo::kSpawnFromModf },
            { "Wmo.RootComplete",              wmo::kRootComplete },
            { "Wmo.CreateMaterial",            wmo::kResolveMaterialTexture },
            { "Wmo.RootWalk",                  wmo::kRootWalk },
            { "Wmo.GroupWalk",                 wmo::kGroupWalk },
            { "Wmo.CullBatch",                 wmo::kCullBatch },
            { "Wmo.LocateViewerMapObjs",       wmo::kLocateViewerMapObjs },
            { "Wmo.PortalTraverse",            wmo::kPortalTraverse },
            { "Wmo.CullSortTable",             wmo::kCullSortTable },
            { "Wmo.CreateOccluders",           wmo::kCreateOccluders },
            { "Wmo.AllocOccluder",             wmo::kAllocOccluder },
            { "Wmo.AddOccluderEdge",           wmo::kAddOccluderEdge },
            { "Wmo.HorizonAabbTest",           wmo::kHorizonAabbTest },
            { "Wmo.GroupParse",                wmo::kGroupParse },
            { "Wmo.ExtRender",                 wmo::kExtRender },
            { "Wmo.IntRender",                 wmo::kIntRender },
            { "Wmo.CompositeEffectBind",       shoff::kEffectBind },
            { "Wmo.GroupComplete",             wmo::kGroupComplete },
            { "Wmo.FreeMapObj",                wmo::kFreeMapObj },
            { "Wmo.FreeMapObjGroup",           wmo::kFreeMapObjGroup },
            { "Wmo.WaitLoad",                  wmo::kWaitLoad },
            { "Wmo.WaitLoadGroup",             wmo::kWaitLoadGroup },
            { "Wmo.UpdateMaterials",           wmo::kUpdateMaterials },
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
