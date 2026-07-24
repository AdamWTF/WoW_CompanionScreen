// Native modern-M2 reader: internal contract shared across the reader's translation units.
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

// The reader is split by the load phases named in NativeLoad.cpp's MECHANISM banner -- demux (M2Demux),
// in-place field deltas + post-fixup injections (M2Fixups), the stock offset->pointer walk (M2Walk) --
// orchestrated by NativeLoad.cpp. The two POD records the phases hand across (Scan from the demux,
// Outcome back to the caller) and the shared byte helpers live here; there is no shared MUTABLE state
// (the session counters are private to the orchestrator, fed from Outcome).

#include "engine/assets/shared/models/m2/M2Format.hpp"

#include <cstdint>
#include <cstring>

namespace wxl::runtime::m2native::detail
{
    inline uint32_t Rd32(const void* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }

    // skipMask bits reported in the OnM2NativeLoad event / logs.
    enum : uint32_t
    {
        kSkipTxac      = 0x01,
        kSkipLdv1      = 0x02,
        kSkipAfid      = 0x04,
        kSkipSkid      = 0x08,
        kSkipPhysBone  = 0x10, // PFID / BFID (no 3.3.5 home, permanently dropped)
        kSkipParticles = 0x20,
        kSkipCameras   = 0x40,
        kSkipOther     = 0x80, // any other auxiliary chunk (EXP2, PFDC, ...)
    };

    constexpr uint32_t kMaxTxid = 128; // corpus max is 7 textures; hard cap for the POD copy

    /// Everything harvested from the container walk, POD so it lives inside the SEH frame.
    struct Scan
    {
        uint32_t bodyOff;             // MD20 body offset within the container
        uint32_t bodySize;
        uint32_t txid[kMaxTxid];
        uint32_t txidCount;
        uint32_t sfidFirst;
        uint32_t sfidCount;
        uint32_t skipMask;
    };

    /// POD outcome of the guarded core, consumed by NativeLoad for stats/event/logging.
    struct Outcome
    {
        int      ok;
        uint32_t version;
        uint32_t texResolved;
        uint32_t texUnresolved;
        uint32_t skipMask;
        uint32_t extSeqPending;
        uint32_t shadowGateForced; // 1 when CM2Shared+0x198 had to be lifted off zero
        uint32_t shadowGateAfter;  // CM2Shared+0x198 read back immediately after the write
        const char* fail; // static failure reason when ok == 0
    };

    // ---------------------------------------------------------------- phase entry points
    /// M2Demux: walks the MD21 container, harvesting the body location and the auxiliary chunks. Returns
    /// true when an MD20 body large enough for a full header was found.
    bool ScanContainer(const uint8_t* buf, uint32_t size, Scan& s);

    /// M2Fixups: sequence deltas in place (split u16 blendTime mask, source-id remap + lookup patch),
    /// counting the .anim-streamed sequences into extSeqPending.
    void FixSequencesRaw(uint8_t* base, uint32_t size, wxl::structure::m2::M2Header* h, uint32_t& extSeqPending);
    /// M2Fixups: material deltas in place (blend-mode clamp into the client's 7-entry table, flag mask).
    void FixMaterialsRaw(uint8_t* base, uint32_t size, wxl::structure::m2::M2Header* h);
    /// M2Fixups: points each hardcoded texture with no inline name at its TXID-resolved client path.
    void InjectTxidNames(wxl::structure::m2::M2Header* h, const Scan& s, Outcome& out);
    /// M2Fixups: clamps each ribbon's texture/material reference values into the header tables.
    void ClampRibbonRefs(wxl::structure::m2::M2Header* h);

    /// M2Walk: drives the stock per-field offset->pointer readers over the modern body in stock order.
    /// Returns true when every reader accepted its array (bounds-valid offsets).
    bool DriveStockWalk(uint8_t* base, uint32_t size, wxl::structure::m2::M2Header* h);
}
