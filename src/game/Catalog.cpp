// Aggregates every binding set's catalog registration into one startup call.
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

#include "game/Catalog.hpp"

#include "client/CMapChunk/Adt.hpp"
#include "client/CGCamera/Camera.hpp"
#include "client/CMapDoodad/Doodad.hpp"
#include "client/SFile/Io.hpp"
#include "client/CM2Shared/M2.hpp"
#include "game/mem/Mem.hpp"
#include "game/ui/Ui.hpp"
#include "client/CMapObj/Wmo.hpp"
#include "client/CWorldScene/Loading.hpp"

namespace wxl::game
{
    /** @brief Registers every binding set's catalog entries. */
    void RegisterAllBindings()
    {
        adt::RegisterCatalog();
        camera::RegisterCatalog();
        doodad::RegisterCatalog();
        io::RegisterCatalog();
        m2::RegisterCatalog();
        mem::RegisterCatalog();
        ui::RegisterCatalog();
        wmo::RegisterCatalog();
        world::RegisterCatalog();
    }
}
