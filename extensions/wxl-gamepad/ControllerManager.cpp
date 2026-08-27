#include "ControllerManager.hpp"
#include "ExtensionApi.hpp"
#include "Processing.hpp"
#include "SdlGamepad.hpp"
#include "XInputBackend.hpp"

#include <windows.h>
#include <algorithm>
#include <chrono>
#include <vector>

namespace wxl_gamepad
{
    namespace
    {
        bool DetectWine()
        {
            HMODULE ntdll = GetModuleHandleA("ntdll.dll"); return ntdll && GetProcAddress(ntdll, "wine_get_version") != nullptr;
        }
    }
    ControllerManager::~ControllerManager() { Stop(); }
    bool ControllerManager::Start()
    {
        wine_ = DetectWine(); g_api->Log(WXL_LOG_INFO, kTag, "[Controller] Runtime: %s", RuntimeName());
        g_api->Log(WXL_LOG_INFO, kTag, "[Controller] Requested backend: %s", BackendName(config_.backend));
        if (!config_.enabled) { Log(WXL_LOG_INFO, "[Controller] disabled by configuration"); return true; }
        stopping_ = false; worker_ = std::thread(&ControllerManager::Run, this); return true;
    }
    void ControllerManager::Stop()
    {
        stopping_ = true; wake_.notify_all(); if (worker_.joinable()) worker_.join();
        if (backend_) { backend_->Shutdown(); backend_.reset(); } PublishDisconnected();
    }
    ControllerSnapshot ControllerManager::Snapshot() const { std::lock_guard lock(mutex_); return snapshot_; }
    std::unique_ptr<IControllerBackend> ControllerManager::Create(BackendKind kind) const
    {
        if (kind == BackendKind::XInput) return std::make_unique<XInputControllerBackend>();
        if (kind == BackendKind::SDL) return std::make_unique<SDLGameControllerBackend>();
        if (kind == BackendKind::SDLJoystick) return std::make_unique<SDLJoystickBackend>(config_.debug);
        return {};
    }
    bool ControllerManager::SelectBackend()
    {
        std::vector<BackendKind> order;
        if (config_.backend != BackendKind::Auto) order.push_back(config_.backend);
        else if (wine_) order = {BackendKind::XInput, BackendKind::SDL, BackendKind::SDLJoystick};
        else order = {BackendKind::SDL, BackendKind::XInput, BackendKind::SDLJoystick};
        if (!diagnosticsDone_)
        {
            for (BackendKind kind : {BackendKind::XInput, BackendKind::SDL, BackendKind::SDLJoystick})
                if (std::find(order.begin(), order.end(), kind) == order.end()) order.push_back(kind);
        }
        std::unique_ptr<IControllerBackend> selected;
        for (BackendKind kind : order)
        {
            auto candidate = Create(kind); if (!candidate || !candidate->Initialize()) continue;
            const bool eligible = config_.backend == BackendKind::Auto || kind == config_.backend;
            if (!selected && eligible && candidate->IsControllerConnected()) selected = std::move(candidate);
            else candidate->Shutdown();
        }
        diagnosticsDone_ = true; if (!selected) return false; backend_ = std::move(selected); const ControllerDeviceInfo info = backend_->GetDeviceInfo();
        { std::lock_guard lock(mutex_); ++snapshot_.generation; snapshot_.connected = true; snapshot_.device = info; snapshot_.backendName = backend_->GetBackendName(); snapshot_.state = {}; ++snapshot_.sequence; }
        g_api->Log(WXL_LOG_INFO, kTag, "[Controller] Selected backend: %s", backend_->GetBackendName()); g_api->Log(WXL_LOG_INFO, kTag, "[Controller] Selected device: %s%s", info.name.c_str(), info.diagnosticOnly ? " (diagnostic only; mapping required)" : ""); haveDebugState_ = false; return true;
    }
    void ControllerManager::PublishDisconnected()
    {
        std::lock_guard lock(mutex_); if (!snapshot_.connected && snapshot_.backendName == "None") return;
        ++snapshot_.generation; ++snapshot_.sequence; snapshot_.connected = false; snapshot_.state = {}; snapshot_.device = {}; snapshot_.backendName = "None";
    }
    void ControllerManager::DebugChanges(const ControllerState& n)
    {
        if (!config_.debug) return;
        struct Named { const char* name; bool ControllerState::*field; } fields[] = {
            {"South", &ControllerState::south}, {"East", &ControllerState::east}, {"West", &ControllerState::west}, {"North", &ControllerState::north},
            {"DPadUp", &ControllerState::dpadUp}, {"DPadDown", &ControllerState::dpadDown}, {"DPadLeft", &ControllerState::dpadLeft}, {"DPadRight", &ControllerState::dpadRight},
            {"LeftShoulder", &ControllerState::leftShoulder}, {"RightShoulder", &ControllerState::rightShoulder}, {"LeftStickButton", &ControllerState::leftStickButton}, {"RightStickButton", &ControllerState::rightStickButton},
        };
        if (haveDebugState_) for (const auto& f : fields) if (n.*(f.field) != debugState_.*(f.field)) g_api->Log(WXL_LOG_INFO, kTag, "[Controller] %s %s", f.name, n.*(f.field) ? "DOWN" : "UP");
        debugState_ = n; haveDebugState_ = true;
    }
    void ControllerManager::Run()
    {
        const auto interval = std::chrono::microseconds(1000000 / config_.pollingRateHz); auto nextScan = std::chrono::steady_clock::now();
        while (!stopping_)
        {
            if (!backend_ && std::chrono::steady_clock::now() >= nextScan) { SelectBackend(); nextScan = std::chrono::steady_clock::now() + std::chrono::seconds(2); }
            if (backend_)
            {
                ControllerState state{};
                if (!backend_->Poll(state) || !backend_->IsControllerConnected())
                {
                    g_api->Log(WXL_LOG_WARN, kTag, "[Controller] %s disconnected", backend_->GetBackendName()); PublishDisconnected(); backend_->Shutdown(); backend_.reset(); nextScan = std::chrono::steady_clock::now();
                }
                else
                {
                    const Stick left = RadialDeadzone(state.leftX, state.leftY, config_.leftStickDeadzone), right = RadialDeadzone(state.rightX, state.rightY, config_.rightStickDeadzone);
                    state.leftX = left.x; state.leftY = left.y; state.rightX = right.x; state.rightY = right.y; DebugChanges(state);
                    std::lock_guard lock(mutex_); snapshot_.state = state; snapshot_.connected = true; ++snapshot_.sequence;
                }
            }
            std::unique_lock lock(waitMutex_); wake_.wait_for(lock, interval, [&] { return stopping_.load(); });
        }
        if (backend_) { backend_->Shutdown(); backend_.reset(); } PublishDisconnected();
    }
}
