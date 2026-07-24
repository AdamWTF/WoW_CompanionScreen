// Compile-time modern-asset support switches.
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

/**
 * @brief Modern retail-asset support switches, one master per asset type.
 *
 * Off => the client falls back to its own stock 3.3.5 parsers for that asset type. Everything else
 * the DLL does (streaming, render, textures, world/unit/sound/char/input hooks, phasing,
 * diagnostics, grass wind, spawn, BLP transcode) is unconditional.
 */
namespace wxl::features
{
    inline constexpr bool modernM2Support  = true; // native M2 (MD21/M3) read-in-place, compat/shadow fixes, M2 memory arena, external .anim
    inline constexpr bool modernWMOSupport = true; // native WMO tag-walkers, MOMT-by-FDID, material/shader fixes, layered material, outdoor gate, MODF collision scale
    inline constexpr bool modernADTSupport = true; // Cata+ split tiles (root/_tex0/_obj0), height blend, MH2O liquid handling
}
