#pragma once

#include "ControllerConfig.hpp"
#include "ControllerTypes.hpp"
#include "GameInput.hpp"

namespace wxl_gamepad
{
    inline int ControllerActionSlot(int layer, int control)
    {
        static constexpr int slots[4][8] = {{1,2,3,4,5,6,7,8}, {9,10,11,12,49,50,51,52}, {53,54,55,56,57,58,59,60}, {61,62,63,64,65,66,67,68}};
        return layer >= 0 && layer < 4 && control >= 0 && control < 8 ? slots[layer][control] : 0;
    }

    class ControllerGameplay final
    {
    public:
        ControllerGameplay(const ControllerConfig& config, IGameInput& input, bool publishState = true) : config_(config), input_(input), publishState_(publishState) {}
        void Update(const ControllerSnapshot& snapshot, float dt, uint32_t time); void Release(uint32_t time);
        int Layer() const { return layer_; } int LastAction() const { return lastAction_; }
        bool LeftModifier() const { return leftTrigger_; } bool RightModifier() const { return rightTrigger_; }
    private:
        void Publish(const ControllerSnapshot& snapshot); void Touch(const ControllerState& state, uint32_t time);
        const ControllerConfig& config_; IGameInput& input_; bool publishState_{}; uint64_t generation_{~uint64_t{}}; ControllerState previous_{};
        bool forward_{}, backward_{}, strafeLeft_{}, strafeRight_{}, leftTrigger_{}, rightTrigger_{}; bool actions_[8]{};
        int layer_{}, reportedLayer_{-1}, lastAction_{}; bool wasForeground_{};
        bool touchWasDown_{}, touchMoved_{}, touchButton_{}, twoFingerCandidate_{}, twoFingerMoved_{}; uint32_t touchStart_{}; float touchStartX_{}, touchStartY_{}, touchLastX_{}, touchLastY_{};
    };
}
