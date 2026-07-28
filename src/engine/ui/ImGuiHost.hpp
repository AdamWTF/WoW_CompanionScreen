// In-game ImGui host: device lifetime, input routing, and a registry of panels to draw.
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

namespace wxl::ui
{
    /// A panel body. Called between NewFrame and Render, only while the overlay is open, so it may
    /// call ImGui freely and must not assume it runs every frame.
    using PanelFn = void (*)(void* user);

    /**
     * @brief Registers a panel to be drawn whenever the overlay is open.
     *
     * Panels register at feature-install time, cold, and are never removed. The host owns no
     * knowledge of what any of them do -- a panel is the only thing a feature needs to write to put
     * controls on screen, and nothing about the device, the input routing or the reset dance is its
     * problem.
     *
     * @param title  window title, also its ImGui identity -- must be unique and stable.
     * @param fn     body, invoked inside an already-open window.
     * @param user   opaque pointer handed back to @p fn.
     */
    void AddPanel(const char* title, PanelFn fn, void* user);

    /// True while the overlay is open and taking input.
    bool IsOpen();
}
