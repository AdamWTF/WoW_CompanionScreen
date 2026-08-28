#pragma once

#include "ControllerConfig.hpp"
#include "ControllerTypes.hpp"
#include "GameInput.hpp"
#include "ThorPadActions.hpp"

namespace wxl_gamepad
{
    class ControllerGameplay final
    {
    public:
        ControllerGameplay(const ControllerConfig& config, IGameInput& input, bool publishState = true) : config_(config), input_(input), publishState_(publishState) {}
        void Update(const ControllerSnapshot& snapshot, float dt, uint32_t time); void SetActive(bool active, uint32_t time); void Release(uint32_t time);
        void ResetSystemActions(uint32_t time); bool SetSystemAction(const char* layer, const char* control, const char* action);
        static bool SupportsSystemAction(const char* action);
        const ThorPadAction& MappedAction(int layer, int control) const { return actionMap_.Get(layer, control); }
        int Layer() const { return layer_; } int LastAction() const { return lastAction_; }
        bool Active() const { return active_; }
        bool LeftModifier() const { return leftTrigger_; } bool RightModifier() const { return rightTrigger_; }
    private:
        void Publish(const ControllerSnapshot& snapshot); void Touch(const ControllerState& state, uint32_t time);
        void Dispatch(const ThorPadAction& action, InputState state, uint32_t time); void ReleaseCapturedActions(uint32_t time);
        const ControllerConfig& config_; IGameInput& input_; bool publishState_{}, active_{}, suppressEdges_{true}; uint64_t generation_{~uint64_t{}}; ControllerState previous_{};
        bool forward_{}, backward_{}, strafeLeft_{}, strafeRight_{}, leftTrigger_{}, rightTrigger_{}; bool actions_[8]{};
        ThorPadAction capturedActions_[8]{}; int capturedSlots_[8]{}; unsigned jumpHolds_{}; ThorPadActionMap actionMap_{};
        int layer_{}, reportedLayer_{-1}, lastAction_{}; bool wasForeground_{};
        bool touchWasDown_{}, touchMoved_{}, touchButton_{}, twoFingerCandidate_{}, twoFingerMoved_{}; uint32_t touchStart_{}; float touchStartX_{}, touchStartY_{}, touchLastX_{}, touchLastY_{};
    };
}
