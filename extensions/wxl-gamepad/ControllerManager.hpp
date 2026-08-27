#pragma once

#include "ControllerConfig.hpp"
#include "ControllerTypes.hpp"
#include "IControllerBackend.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace wxl_gamepad
{
    class ControllerManager final
    {
    public:
        explicit ControllerManager(const ControllerConfig& config) : config_(config) {}
        ~ControllerManager();
        bool Start(); void Stop(); ControllerSnapshot Snapshot() const;
        bool IsWine() const { return wine_; } const char* RuntimeName() const { return wine_ ? "Wine" : "Native Windows"; }
    private:
        void Run(); bool SelectBackend(); std::unique_ptr<IControllerBackend> Create(BackendKind kind) const; void PublishDisconnected(); void DebugChanges(const ControllerState& next);
        const ControllerConfig& config_; mutable std::mutex mutex_; std::mutex waitMutex_; std::condition_variable wake_;
        ControllerSnapshot snapshot_{}; std::unique_ptr<IControllerBackend> backend_; std::thread worker_; std::atomic<bool> stopping_{};
        bool wine_{}, haveDebugState_{}, diagnosticsDone_{}; ControllerState debugState_{};
    };
}
