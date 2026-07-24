// Disk-queue parallelism: run three background reader workers instead of one, routing reads round-robin.
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

// The engine ships one background reader thread draining a single queue: every asset read -- terrain,
// models, textures -- serialises through it, so a fast flight or a zone-in stalls behind one disk
// worker. The queue-slot table and the worker machinery already accommodate three queues (a shape the
// engine only ever fills in one configuration it does not ship); this feature fills the two spare slots
// with real workers and spreads enqueued reads across all three. Both halves are in-process, no host
// involvement -- they extend the engine's own reader.

#include "offsets/engine/Async.hpp"

#include "engine/hook/Hook.hpp"
#include "engine/hook/Registry.hpp"
#include "common/Config.hpp"
#include "common/Log.hpp"

#include <cstdint>
#include <mutex>

namespace asyncio = wxl::offsets::engine::asyncio;

namespace
{
    /// The reader-queue slot table (kQueueSlotCount contiguous queue pointers).
    void** Slots() { return reinterpret_cast<void**>(asyncio::kQueueTable); }

    // TEMP diagnostic: proves reads are genuinely spread across the workers, not routed to one queue.
    // Remove after in-game validation. Silence at runtime with WXL_DISKQ_LOG=0.
    bool DiagLog() { static const bool on = wxl::config::Env("WXL_DISKQ_LOG", true); return on; }

    asyncio::InitializeFn  g_origInitialize  = nullptr;
    asyncio::EnqueueReadFn g_origEnqueueRead = nullptr;

    // OS thread names for the two added workers (cosmetic; they run the identical reader body).
    const char* const kAddedLabels[] = { "WXL Disk Queue 2", "WXL Disk Queue 3" };

    /**
     * @brief Runs the stock reader setup, then fills any empty queue slot with a queue + worker thread.
     *
     * The stock setup populates only slot 0 in the shipping configuration, leaving the rest null. Each
     * still-null slot gets a freshly allocated queue and a started worker built from the exact
     * allocation + spawn primitives the stock setup uses for slot 0 -- so the added queues/threads are
     * byte-identical in shape and self-register into the teardown lists the engine already walks, needing
     * no shutdown changes. A slot the stock setup did populate is left untouched.
     */
    void __cdecl InitializeDetour(uint32_t maxReadsPerTick, uint32_t pumpBudgetMs)
    {
        g_origInitialize(maxReadsPerTick, pumpBudgetMs);

        auto allocQueue  = reinterpret_cast<asyncio::AllocQueueFn>(asyncio::kAllocQueue);
        auto spawnWorker = reinterpret_cast<asyncio::SpawnWorkerFn>(asyncio::kSpawnWorker);
        void** slots = Slots();

        uint32_t added = 0;
        for (uint32_t i = 1; i < asyncio::kQueueSlotCount; ++i)
        {
            if (slots[i]) continue; // already populated -- leave as-is
            void* queue = allocQueue();
            if (!queue) break;
            slots[i] = queue;
            spawnWorker(queue, kAddedLabels[i - 1]);
            ++added;
        }
        // Log the resolved slots: three distinct non-null pointers is proof of three real, separate
        // queues (rules out an alloc that handed back one shared object).
        WLOG_INFO("disk-queue: %u reader worker(s) total (%u added); slots [0]=%p [1]=%p [2]=%p",
                  1u + added, added, slots[0], slots[1], slots[2]);
    }

    // Serialises the slot swap below against every other enqueue. The stock enqueue is already a short
    // locked critical section, so widening it to cover the swap adds negligible contention -- the
    // parallelism win is on the worker threads doing the reads, not at the enqueue point.
    std::mutex g_routeMutex;
    uint32_t   g_next = 0; // round-robin cursor

    // TEMP diagnostic counters (guarded by g_routeMutex): per-slot enqueue totals, dumped periodically.
    uint64_t g_routed[asyncio::kQueueSlotCount] = {};
    uint64_t g_routeTotal = 0;

    /**
     * @brief Routes each read request across all live reader queues round-robin.
     *
     * The stock enqueue reads the default slot (slot 0) and threads that queue through its whole locked
     * insert. Rather than reproduce that insert, this temporarily points slot 0 at the chosen queue,
     * runs the stock enqueue unmodified, then restores slot 0 -- all under one lock so no concurrent
     * enqueue observes the swap. The stock enqueue is the only runtime reader of the slot table (the
     * workers hold their own queue fixed at creation), so nothing else can see the transient value.
     * Self-adapting: with only slot 0 live, every request routes there exactly as stock.
     */
    void __cdecl EnqueueReadDetour(void* request, uint32_t insertMode)
    {
        void** slots = Slots();
        uint64_t snap[asyncio::kQueueSlotCount];
        bool dump = false;
        {
            std::lock_guard<std::mutex> lock(g_routeMutex);

            uint32_t live = 1;
            while (live < asyncio::kQueueSlotCount && slots[live]) ++live;

            const uint32_t pick = g_next++ % live;
            void* saved = slots[0];
            slots[0] = slots[pick];
            g_origEnqueueRead(request, insertMode);
            slots[0] = saved;

            if (DiagLog())
            {
                ++g_routed[pick];
                if ((++g_routeTotal & 0xFF) == 0) // every 256 requests
                {
                    for (uint32_t i = 0; i < asyncio::kQueueSlotCount; ++i) snap[i] = g_routed[i];
                    dump = true;
                }
            }
        }
        if (dump)
            WLOG_INFO("disk-queue routed so far: q0=%llu q1=%llu q2=%llu",
                      static_cast<unsigned long long>(snap[0]),
                      static_cast<unsigned long long>(snap[1]),
                      static_cast<unsigned long long>(snap[2]));
    }

    /**
     * @brief Installs the two reader detours. Registered at Boot so both are live before the engine
     *        reaches its own reader setup.
     */
    bool Install()
    {
        wxl::hook::Install("disk-queue-init",  asyncio::kInitialize,  &InitializeDetour,  &g_origInitialize);
        wxl::hook::Install("disk-queue-route", asyncio::kEnqueueRead, &EnqueueReadDetour, &g_origEnqueueRead);
        return true;
    }
}

WXL_REGISTER_FEATURE_PHASED("disk-queue-parallel", true, Install, ::wxl::hook::Phase::Boot)
