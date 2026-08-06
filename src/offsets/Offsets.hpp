// Single include surface over the core's internal offset tables.
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

// INTERNAL to the core. Addresses and field offsets into the live client image / engine objects
// (distinct from the on-disk file contracts in asset/*/). Modules never include this; they use
// wxl::game / wxl::events.
#include "offsets/engine/Draw.hpp"
#include "offsets/engine/Gx.hpp"
#include "offsets/engine/Io.hpp"
#include "offsets/engine/Mem.hpp"
#include "offsets/game/ADT.hpp"
#include "offsets/game/DB2.hpp"
#include "offsets/game/M2.hpp"
#include "offsets/game/Unit.hpp"
#include "offsets/game/WMO.hpp"
#include "offsets/game/World.hpp"
