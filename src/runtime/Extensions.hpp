// Discovery and loading of out-of-core extensions.
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

namespace wxl::runtime::extensions
{
    /**
     * @brief Loads every extension under Extensions/, in name order.
     *
     * One folder per extension holding a DLL of the same name, mirroring the layout the client uses
     * for its own script addons. Each is queried before any of its code runs and turned away on a
     * version it did not compile against; a refusal or a failure never stops the others.
     *
     * Called on the main thread once the graphics device exists and before the detour batch is
     * armed, so an extension's detours are enabled together with the core's own.
     * @return the number of extensions that loaded.
     */
    int LoadAll();
}
