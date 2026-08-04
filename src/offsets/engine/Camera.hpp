// World camera matrix globals and the per-frame matrix builder address.
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

// INTERNAL to the core. The world camera matrices, as static globals (the image is fixed-base, no
// ASLR, so they are read directly). float[16], row-major, D3D row-vector convention. They are valid
// only in-world: at the login / loading screen they sit at identity with the camera at the origin.
namespace wxl::offsets::engine::camera
{
    constexpr uintptr_t kProjection = 0x00ADF628;
    constexpr uintptr_t kView       = 0x00ADF5E8; // world -> view
    constexpr uintptr_t kViewProj   = 0x00ADF460;
    constexpr uintptr_t kCameraPos  = 0x00CD8F5C; // vec3

    // Per-frame builder that recomputes the view/projection from the camera state.
    constexpr uintptr_t kBuildCameraMatrices = 0x00795400;

    // --- the camera the world renderer reads ---
    // CGWorldFrame::GetActiveCamera returns *(worldFrame + 0x7E20), or null when there is no world
    // frame. The world scene render calls it and immediately calls a virtual on the result, so this is
    // where a scene rendered outside the world has to supply one.
    constexpr uintptr_t kGetActiveCamera = 0x004F5960;
    using GetActiveCameraFn = void*(__cdecl*)();

    // CSimpleCamera's vtable, exactly four entries: field of view, forward, right, up. Each is a single
    // field read, so an object laid out as below and pointed at this vtable answers all four with the
    // engine's own implementations rather than a hand-written stand-in.
    constexpr uintptr_t kSimpleCameraVTable = 0x00A1E864;
    constexpr size_t kCameraPosition = 0x08; // float[3]
    constexpr size_t kCameraForward  = 0x14; // float[3]
    constexpr size_t kCameraRight    = 0x20; // float[3]
    constexpr size_t kCameraUp       = 0x2C; // float[3]
    constexpr size_t kCameraFov      = 0x40; // float, full angle in radians

#pragma pack(push, 1)
    /** @brief The part of a camera the world renderer reads, at the offsets its methods use. */
    struct SimpleCamera
    {
        const void* vtable;                                  // 0x00
        uint8_t     _pad04[kCameraPosition - sizeof(void*)];
        float       position[3];                             // kCameraPosition
        float       forward[3];                              // kCameraForward
        float       right[3];                                // kCameraRight
        float       up[3];                                   // kCameraUp
        uint8_t     _pad38[kCameraFov - (kCameraUp + 12)];
        float       fov;                                     // kCameraFov
        // Slack for the fields past the four methods, which are not mapped: the engine only ever sees
        // this object through them, but it must not be shorter than what it claims to be.
        uint8_t     _tail[0x40];
    };
    static_assert(offsetof(SimpleCamera, position) == kCameraPosition, "SimpleCamera.position");
    static_assert(offsetof(SimpleCamera, forward)  == kCameraForward,  "SimpleCamera.forward");
    static_assert(offsetof(SimpleCamera, right)    == kCameraRight,    "SimpleCamera.right");
    static_assert(offsetof(SimpleCamera, up)       == kCameraUp,       "SimpleCamera.up");
    static_assert(offsetof(SimpleCamera, fov)      == kCameraFov,      "SimpleCamera.fov");
#pragma pack(pop)
}
