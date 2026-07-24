// Background asynchronous file-reader addresses: queue setup, read routing, and the queue-slot table.
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

// INTERNAL to the core. The engine's background file-reader: a boot-time setup routine spins up the
// reader queue(s) + worker thread(s), an enqueue routine routes each read request onto a queue, and a
// small pointer table holds the live queues. Modules never include this; they use wxl::game / wxl::events.
namespace wxl::offsets::engine::asyncio
{
    // Boot-time setup: allocates the reader queue(s) and starts their worker thread(s). Runs once,
    // early in client startup. In the shipping configuration it populates a single queue slot.
    constexpr uintptr_t kInitialize = 0x004BAA40;
    using InitializeFn = void(__cdecl*)(uint32_t maxReadsPerTick, uint32_t pumpBudgetMs);

    // Enqueue+route: places one read request onto a reader queue (reads the default slot, then does a
    // locked priority-sorted insert). insertMode selects the insert variant.
    constexpr uintptr_t kEnqueueRead = 0x004BAB50;
    using EnqueueReadFn = void(__cdecl*)(void* request, uint32_t insertMode);

    // Allocates one reader-queue object (0x24 bytes, zero-initialised) and registers it into the
    // engine's live-queue tracking list. Returns the queue, or null on allocation failure.
    constexpr uintptr_t kAllocQueue = 0x004BA8E0;
    using AllocQueueFn = void*(__cdecl*)();

    // Wraps a reader queue in a worker record and starts its reader thread; the label is the OS thread
    // name (cosmetic). The worker self-registers into the engine's live-thread tracking list.
    constexpr uintptr_t kSpawnWorker = 0x004BA980;
    using SpawnWorkerFn = void(__cdecl*)(void* queue, const char* label);

    // The reader-queue slot table: contiguous pointer slots, one per live queue. Slots past the first
    // are null in the shipping configuration; the table has room for exactly kQueueSlotCount.
    constexpr uintptr_t kQueueTable     = 0x00B4A20C;
    constexpr uint32_t  kQueueSlotCount = 3;
}
