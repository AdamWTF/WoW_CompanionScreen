#pragma once

#include "SmartInteract.hpp"

namespace wxl_gamepad
{
    class NativeSmartInteractWorld final : public ISmartInteractWorld
    {
    public:
        std::optional<SmartInteractCandidate> CurrentTarget() override;
        SmartInteractVec3 PlayerPosition() override;
        SmartInteractVec3 ViewForward() override;
        bool InCombat() override;
        std::vector<SmartInteractCandidate> Candidates(float radius) override;
        bool Target(uint64_t guid) override;
        bool Interact(uint64_t guid) override;
        void DebugCandidate(const SmartInteractCandidate&, const SmartInteractScore&, bool) override;
        void DebugResult(SmartInteractResult) override;
    private:
        void* Player() const;
        std::optional<SmartInteractCandidate> Describe(uint64_t guid, void* object, void* player) const;
    };
}
