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
     * @brief Arms the detour that loads the extensions.
     *
     * Called from DllMain, where LoadLibrary is not an option: the loading itself happens later,
     * from the engine-initialisation seam, on the main thread and outside the loader lock. That
     * point still precedes the reader-queue setup and the texture scratch sizing, so an extension
     * can reach either.
     *
     * Extensions load one folder per extension out of Extensions/, in name order, each holding a DLL
     * of the same name -- the layout the client already uses for its own addons. Each is queried
     * before any of its code runs; a refusal or a failure never stops the others.
     * @return true if the detour was registered.
     */
    bool InstallLoader();
}
