// Native player-input control addresses for WoW 3.3.5a build 12340.
// Copyright (C) 2026 WarcraftXL contributors. GPL-3.0-or-later.
#pragma once

#include <cstdint>

// INTERNAL to the core. These were derived from the stock action handlers in Wow.exe SHA-256
// B8BD1A0DA194A4098B32D2B6F5798286CD1B07CDEEBDB0E7A1424235CB9DD379:
//
// MoveForwardStart at 0x005FC200 loads [0x00C24954], calls Begin(control=0x10,time), then Commit.
// Its stop twin calls End(control=0x10,time,0), then Commit. Back/strafe handlers use the same
// pattern with 0x20/0x40/0x80. JumpOrAscendStart uses 0x2000 and takes its timestamp from
// [0x00B499A4]. The handlers' preceding
// 0x005191C0 hardware/protected-action check is intentionally not part of this binding: extensions
// call the already-validated native input-control functions directly on the game thread.
namespace wxl::offsets::engine::input
{
    constexpr uintptr_t kControl = 0x00C24954; // CInputControl*, confirmed by all stock action handlers
    constexpr uintptr_t kActionTime = 0x00B499A4; // client action timestamp used by all stock input handlers
    constexpr uintptr_t kBegin   = 0x005FA170; // __thiscall(control, controlBit, timeMs), returns changed
    constexpr uintptr_t kEnd     = 0x005FA450; // __thiscall(control, controlBit, timeMs, 0), returns changed
    constexpr uintptr_t kCommit  = 0x005FBBC0; // __thiscall(control, timeMs, sendMovement=1)

    using BeginFn  = int (__thiscall*)(void* control, uint32_t controlBit, uint32_t timeMs);
    using EndFn    = int (__thiscall*)(void* control, uint32_t controlBit, uint32_t timeMs, int unknown);
    using CommitFn = void(__thiscall*)(void* control, uint32_t timeMs, int sendMovement);

    enum class Control : uint32_t
    {
        Forward     = 0x0010,
        Backward    = 0x0020,
        StrafeLeft  = 0x0040,
        StrafeRight = 0x0080,
        TurnLeft    = 0x0100,
        TurnRight   = 0x0200,
        Jump        = 0x2000,
    };
}
