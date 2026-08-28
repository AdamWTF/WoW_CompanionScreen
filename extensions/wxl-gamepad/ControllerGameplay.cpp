#include "ControllerGameplay.hpp"
#include "ExtensionApi.hpp"
#include "Processing.hpp"

#include "game/Script.hpp"

#include <cmath>
#include <cstdio>

namespace wxl_gamepad
{
    namespace
    {
        std::string LuaSafe(std::string text) { for (char& c : text) if (c == '\'' || c == '\\' || c == '\r' || c == '\n') c = '_'; return text; }
    }
    void ControllerGameplay::Dispatch(const ThorPadAction& action, InputState state, uint32_t time)
    {
        if (action.type == ThorPadActionType::WoWAction)
        {
            if (state == InputState::Pressed) input_.WoWAction(action.wowActionSlot);
            return;
        }
        if (action.type != ThorPadActionType::SystemAction || action.systemAction == ThorPadSystemAction::Unknown) return;
        if (action.systemAction == ThorPadSystemAction::Jump)
        {
            if (state == InputState::Pressed)
            {
                if (jumpHolds_++ == 0) input_.SystemAction(action.systemAction, state, time);
            }
            else if (jumpHolds_ && --jumpHolds_ == 0) input_.SystemAction(action.systemAction, state, time);
        }
        else if (action.systemAction == ThorPadSystemAction::Interact && state == InputState::Pressed)
            input_.SystemAction(action.systemAction, state, time);
    }
    void ControllerGameplay::ReleaseCapturedActions(uint32_t time)
    {
        for (int index = 0; index < 8; ++index) { Dispatch(capturedActions_[index], InputState::Released, time); capturedActions_[index] = {}; capturedSlots_[index] = 0; }
        jumpHolds_ = 0;
    }
    void ControllerGameplay::ResetUIRepeat()
    {
        uiDirection_ = -1; uiRepeatAt_ = 0; for (bool& direction : uiDirections_) direction = false;
    }
    bool ControllerGameplay::IsNeutral(const ControllerState& s) const
    {
        return !s.south && !s.east && !s.west && !s.north && !s.dpadUp && !s.dpadDown && !s.dpadLeft && !s.dpadRight &&
            !s.leftShoulder && !s.rightShoulder && !s.leftStickButton && !s.rightStickButton && !s.start && !s.back &&
            s.leftTrigger < config_.triggerPressThreshold && s.rightTrigger < config_.triggerPressThreshold &&
            s.leftX == 0 && s.leftY == 0 && s.rightX == 0 && s.rightY == 0;
    }
    void ControllerGameplay::SetUINavigationActive(bool active, uint32_t time)
    {
        active = active && active_;
        if (uiNavigation_ == active) return;
        ReleaseCapturedActions(time); input_.ReleaseAll(time); forward_ = backward_ = strafeLeft_ = strafeRight_ = false; leftTrigger_ = rightTrigger_ = false;
        touchWasDown_ = touchMoved_ = touchButton_ = twoFingerCandidate_ = twoFingerMoved_ = false;
        uiNavigation_ = active; waitForNeutral_ = true; suppressEdges_ = true; ResetUIRepeat(); for (bool& action : actions_) action = false;
    }
    void ControllerGameplay::ResetSystemActions(uint32_t time) { ReleaseCapturedActions(time); actionMap_.Reset(); }
    bool ControllerGameplay::SetSystemAction(const char* layer, const char* control, const char* action)
    {
        if (!layer || !control || !action) return false;
        const bool supported = actionMap_.SetSystemAction(layer, control, action);
        if (!supported && g_api) g_api->Log(WXL_LOG_WARN, kTag, "unknown or invalid System Action mapping: layer=%s control=%s action=%s", layer, control, action);
        return supported;
    }
    bool ControllerGameplay::SupportsSystemAction(const char* action) { return action && ParseSystemAction(action) != ThorPadSystemAction::Unknown; }
    void ControllerGameplay::Release(uint32_t time)
    {
        ReleaseCapturedActions(time); input_.ReleaseAll(time); forward_ = backward_ = strafeLeft_ = strafeRight_ = false; leftTrigger_ = rightTrigger_ = false; layer_ = 0; reportedLayer_ = -1; touchWasDown_ = touchMoved_ = touchButton_ = twoFingerCandidate_ = twoFingerMoved_ = false; for (bool& action : actions_) action = false; previous_ = {}; waitForNeutral_ = true; ResetUIRepeat();
    }
    void ControllerGameplay::SetActive(bool active, uint32_t time)
    {
        if (active_ == active) return;
        Release(time); active_ = active; if (!active) uiNavigation_ = false; suppressEdges_ = true;
    }
    void ControllerGameplay::Publish(const ControllerSnapshot& snapshot)
    {
        if (!publishState_) return;
        char script[768]; const std::string device = LuaSafe(snapshot.device.name), backend = LuaSafe(snapshot.backendName), hint = LuaSafe(snapshot.device.glyphHint), configured = LuaSafe(config_.glyphStyle);
        std::snprintf(script, sizeof script, "WXLGamepadNativeLayer=%d;WXLGamepadNativeConnected=%s;WXLGamepadNativeBackend='%s';WXLGamepadNativeDevice='%s';WXLGamepadDetectedGlyphStyle='%s';WXLGamepadConfiguredGlyphStyle='%s';WXLGamepadIsAvailable=function() return true end", layer_ + 1, snapshot.connected ? "true" : "false", backend.c_str(), device.c_str(), hint.c_str(), configured.c_str()); wxl::game::script::Execute(script);
    }
    void ControllerGameplay::Touch(const ControllerState& state, uint32_t time)
    {
        constexpr float travel = .035f; constexpr uint32_t tapMs = 220;
        if (!state.touchSupported) return;
        if (state.touchFingers == 2) { if (!twoFingerCandidate_) { twoFingerCandidate_ = true; twoFingerMoved_ = false; touchStart_ = time; touchStartX_ = state.touchX; touchStartY_ = state.touchY; } float dx = state.touchX - touchStartX_, dy = state.touchY - touchStartY_; if (dx*dx + dy*dy > travel*travel) twoFingerMoved_ = true; }
        else if (state.touchFingers > 2 || (twoFingerCandidate_ && state.touchFingers == 1)) twoFingerCandidate_ = false;
        else if (!state.touchFingers && twoFingerCandidate_) { if (!twoFingerMoved_ && time - touchStart_ <= tapMs) input_.PointerClick(true); twoFingerCandidate_ = false; }
        if (state.touchDown)
        {
            if (touchWasDown_) input_.PointerMove(int((state.touchX - touchLastX_) * 1000), int((state.touchY - touchLastY_) * 700));
            else { touchStart_ = time; touchStartX_ = state.touchX; touchStartY_ = state.touchY; touchMoved_ = false; }
            float dx = state.touchX - touchStartX_, dy = state.touchY - touchStartY_; if (dx*dx + dy*dy > travel*travel) touchMoved_ = true; touchLastX_ = state.touchX; touchLastY_ = state.touchY;
        }
        else if (touchWasDown_ && !state.touchFingers && !touchMoved_ && time - touchStart_ <= tapMs) input_.PointerClick(false);
        touchWasDown_ = state.touchDown; if (state.touchpadButton && !touchButton_) input_.PointerClick(false); touchButton_ = state.touchpadButton;
    }
    void ControllerGameplay::UpdateUINavigation(const ControllerState& s, uint32_t time)
    {
        const bool directions[4] = {s.dpadUp, s.dpadDown, s.dpadLeft, s.dpadRight};
        static constexpr UINavigationCommand commands[4] = {UINavigationCommand::Up, UINavigationCommand::Down, UINavigationCommand::Left, UINavigationCommand::Right};
        int newest = -1;
        for (int i = 0; i < 4; ++i) if (directions[i] && !uiDirections_[i]) newest = i;
        const bool opposing = (directions[0] && directions[1]) || (directions[2] && directions[3]);
        if (!opposing && newest >= 0) { uiDirection_ = newest; input_.UINavigation(commands[newest]); uiRepeatAt_ = time + 350; }
        if (uiDirection_ >= 0 && !directions[uiDirection_])
        {
            uiDirection_ = -1; for (int i = 0; i < 4; ++i) if (directions[i]) uiDirection_ = i; uiRepeatAt_ = uiDirection_ >= 0 ? time + 350 : 0;
        }
        if (opposing) { uiDirection_ = -1; uiRepeatAt_ = 0; }
        if (uiDirection_ >= 0 && static_cast<int32_t>(time - uiRepeatAt_) >= 0) { input_.UINavigation(commands[uiDirection_]); uiRepeatAt_ = time + 100; }
        for (int i = 0; i < 4; ++i) uiDirections_[i] = directions[i];
        if (s.south && !previous_.south) input_.UINavigation(UINavigationCommand::Confirm);
        if (s.east && !previous_.east) { input_.UINavigation(UINavigationCommand::Back); if (!uiNavigation_) return; }
        if (s.start && !previous_.start) { input_.Command(GameCommand::ToggleGameMenu); if (!uiNavigation_) return; }
        if (s.back && !previous_.back) input_.Command(GameCommand::ToggleAllBags);
    }
    void ControllerGameplay::Update(const ControllerSnapshot& snapshot, float dt, uint32_t time)
    {
        if (!active_) return;
        const bool generationChanged = snapshot.generation != generation_;
        if (generationChanged) { Release(time); generation_ = snapshot.generation; reportedLayer_ = -1; suppressEdges_ = true; Publish(snapshot); }
        if (!snapshot.connected) return;
        if (suppressEdges_)
        {
            previous_ = snapshot.state; const ControllerState& held = snapshot.state;
            actions_[0]=held.dpadUp; actions_[1]=held.dpadDown; actions_[2]=held.dpadLeft; actions_[3]=held.dpadRight; actions_[4]=held.south; actions_[5]=held.east; actions_[6]=held.west; actions_[7]=held.north;
            suppressEdges_ = false;
        }
        const bool foreground = input_.Foreground(); if (!foreground) { if (wasForeground_) Release(time); previous_ = snapshot.state; const ControllerState& held = snapshot.state; actions_[0]=held.dpadUp; actions_[1]=held.dpadDown; actions_[2]=held.dpadLeft; actions_[3]=held.dpadRight; actions_[4]=held.south; actions_[5]=held.east; actions_[6]=held.west; actions_[7]=held.north; wasForeground_ = false; return; } wasForeground_ = true;
        const ControllerState& s = snapshot.state;
        if (waitForNeutral_) { previous_ = s; if (IsNeutral(s)) { waitForNeutral_ = false; previous_ = {}; } return; }
        if (uiNavigation_) { UpdateUINavigation(s, time); if (!waitForNeutral_) Touch(s, time); previous_ = s; return; }
        leftTrigger_ = Hysteresis(s.leftTrigger, leftTrigger_, config_.triggerPressThreshold, config_.triggerReleaseThreshold); rightTrigger_ = Hysteresis(s.rightTrigger, rightTrigger_, config_.triggerPressThreshold, config_.triggerReleaseThreshold); layer_ = (leftTrigger_ ? 1 : 0) | (rightTrigger_ ? 2 : 0);
        forward_ = DirectionHysteresis(-s.leftY, forward_, config_.movementPressThreshold, config_.movementReleaseThreshold); backward_ = DirectionHysteresis(s.leftY, backward_, config_.movementPressThreshold, config_.movementReleaseThreshold); strafeLeft_ = DirectionHysteresis(-s.leftX, strafeLeft_, config_.movementPressThreshold, config_.movementReleaseThreshold); strafeRight_ = DirectionHysteresis(s.leftX, strafeRight_, config_.movementPressThreshold, config_.movementReleaseThreshold);
        input_.Movement(MovementControl::Forward, forward_, time); input_.Movement(MovementControl::Backward, backward_, time); input_.Movement(MovementControl::StrafeLeft, strafeLeft_, time); input_.Movement(MovementControl::StrafeRight, strafeRight_, time);
        const bool camera = s.rightX != 0 || s.rightY != 0; float dx = Response(s.rightX, config_.cameraResponseCurve) * config_.cameraSensitivityX * config_.cameraMaxPixelsPerSecond * dt; float dy = Response(s.rightY, config_.cameraResponseCurve) * config_.cameraSensitivityY * config_.cameraMaxPixelsPerSecond * dt * (config_.invertCameraY ? -1.0f : 1.0f); input_.Camera(camera, dx, dy);
        if (s.leftShoulder && !previous_.leftShoulder) input_.Target(config_.previousHostile); if (s.rightShoulder && !previous_.rightShoulder) input_.Target(config_.nextHostile);
        bool toggledInterface = false;
        if (s.start && !previous_.start) { input_.Command(GameCommand::ToggleGameMenu); toggledInterface = true; }
        if (s.back && !previous_.back) { input_.Command(GameCommand::ToggleAllBags); toggledInterface = true; }
        if (toggledInterface || uiNavigation_) { previous_ = s; return; }
        if (s.leftStickButton && !previous_.leftStickButton) input_.Command(GameCommand::NextView); if (s.rightStickButton && !previous_.rightStickButton) input_.Target(config_.nextFriendly);
        const bool buttons[8] = {s.dpadUp,s.dpadDown,s.dpadLeft,s.dpadRight,s.south,s.east,s.west,s.north};
        for (int i = 0; i < 8; ++i)
        {
            if (buttons[i] && !actions_[i])
            {
                capturedActions_[i] = actionMap_.Get(layer_, i); capturedSlots_[i] = ControllerActionSlot(layer_, i); Dispatch(capturedActions_[i], InputState::Pressed, time); lastAction_ = capturedSlots_[i];
                if (uiNavigation_) { previous_ = s; return; }
                if (config_.debug) g_api->Log(WXL_LOG_INFO, kTag, "control=%d DOWN layer=%d slot=%d action=%s:%s", i, layer_ + 1, lastAction_, capturedActions_[i].type == ThorPadActionType::SystemAction ? "SYSTEM" : "WOW", capturedActions_[i].type == ThorPadActionType::SystemAction ? SystemActionName(capturedActions_[i].systemAction) : "ACTION");
            }
            else if (!buttons[i] && actions_[i])
            {
                const ThorPadAction released = capturedActions_[i]; const int releasedSlot = capturedSlots_[i]; Dispatch(released, InputState::Released, time); capturedActions_[i] = {}; capturedSlots_[i] = 0;
                if (config_.debug) g_api->Log(WXL_LOG_INFO, kTag, "control=%d UP slot=%d action=%s:%s", i, releasedSlot, released.type == ThorPadActionType::SystemAction ? "SYSTEM" : "WOW", released.type == ThorPadActionType::SystemAction ? SystemActionName(released.systemAction) : "ACTION");
            }
            actions_[i] = buttons[i];
        }
        Touch(s, time); previous_ = s; if (layer_ != reportedLayer_) { reportedLayer_ = layer_; Publish(snapshot); }
    }
}
