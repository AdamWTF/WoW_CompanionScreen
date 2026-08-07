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

#include "offsets/engine/Mem.hpp"
#include "offsets/game/ADT.hpp"
#include "offsets/game/M2.hpp"
#include "offsets/game/World.hpp"

#include <cstring>

namespace wxl::runtime::hookpoints
{
    namespace
    {
        namespace adt = wxl::offsets::game::adt;
        namespace mem = wxl::offsets::engine::mem;
        namespace m2  = wxl::offsets::game::m2;
        namespace wld = wxl::offsets::game::world;

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
            { "World.AsyncFileReadInitialize", wld::kAsyncFileReadInitialize },
            { "World.AsyncFileReadObject",     wld::kAsyncFileReadObject },
            { "World.ValidateFarClip",         wld::kValidateFarClip },
            { "Mem.SMemAlloc",                 mem::kAlloc },
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
