#include "ControllerGameplay.hpp"
#include "ExtensionApi.hpp"
#include "Processing.hpp"

#include "game/Action.hpp"
#include "game/Script.hpp"

#include <cmath>
#include <cstdio>

namespace wxl_gamepad
{
    namespace
    {
        std::string LuaSafe(std::string text) { for (char& c : text) if (c == '\'' || c == '\\' || c == '\r' || c == '\n') c = '_'; return text; }
    }
    void ControllerGameplay::Release(uint32_t time)
    {
        input_.ReleaseAll(time); forward_ = backward_ = strafeLeft_ = strafeRight_ = false; leftTrigger_ = rightTrigger_ = false; layer_ = 0; reportedLayer_ = -1; touchWasDown_ = touchMoved_ = touchButton_ = twoFingerCandidate_ = twoFingerMoved_ = false; for (bool& action : actions_) action = false; previous_ = {};
    }
    void ControllerGameplay::SetActive(bool active, uint32_t time)
    {
        if (active_ == active) return;
        Release(time); active_ = active; suppressEdges_ = true;
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
        leftTrigger_ = Hysteresis(s.leftTrigger, leftTrigger_, config_.triggerPressThreshold, config_.triggerReleaseThreshold); rightTrigger_ = Hysteresis(s.rightTrigger, rightTrigger_, config_.triggerPressThreshold, config_.triggerReleaseThreshold); layer_ = (leftTrigger_ ? 1 : 0) | (rightTrigger_ ? 2 : 0);
        forward_ = DirectionHysteresis(-s.leftY, forward_, config_.movementPressThreshold, config_.movementReleaseThreshold); backward_ = DirectionHysteresis(s.leftY, backward_, config_.movementPressThreshold, config_.movementReleaseThreshold); strafeLeft_ = DirectionHysteresis(-s.leftX, strafeLeft_, config_.movementPressThreshold, config_.movementReleaseThreshold); strafeRight_ = DirectionHysteresis(s.leftX, strafeRight_, config_.movementPressThreshold, config_.movementReleaseThreshold);
        input_.Movement(MovementControl::Forward, forward_, time); input_.Movement(MovementControl::Backward, backward_, time); input_.Movement(MovementControl::StrafeLeft, strafeLeft_, time); input_.Movement(MovementControl::StrafeRight, strafeRight_, time);
        const bool camera = s.rightX != 0 || s.rightY != 0; float dx = Response(s.rightX, config_.cameraResponseCurve) * config_.cameraSensitivityX * config_.cameraMaxPixelsPerSecond * dt; float dy = Response(s.rightY, config_.cameraResponseCurve) * config_.cameraSensitivityY * config_.cameraMaxPixelsPerSecond * dt * (config_.invertCameraY ? -1.0f : 1.0f); input_.Camera(camera, dx, dy);
        if (s.leftShoulder && !previous_.leftShoulder) input_.Target(config_.previousHostile); if (s.rightShoulder && !previous_.rightShoulder) input_.Target(config_.nextHostile);
        if (s.start && !previous_.start) input_.Command(GameCommand::ToggleGameMenu); if (s.back && !previous_.back) input_.Command(GameCommand::ToggleAllBags);
        if (s.leftStickButton && !previous_.leftStickButton) input_.Command(GameCommand::NextView); if (s.rightStickButton && !previous_.rightStickButton) input_.Target(config_.nextFriendly);
        const bool buttons[8] = {s.dpadUp,s.dpadDown,s.dpadLeft,s.dpadRight,s.south,s.east,s.west,s.north};
        for (int i = 0; i < 8; ++i) { const int slot = ControllerActionSlot(layer_, i); if (buttons[i] && !actions_[i] && wxl::game::action::Use(slot)) { lastAction_ = slot; if (config_.debug) g_api->Log(WXL_LOG_INFO, kTag, "action edge: layer=%d slot=%d", layer_ + 1, lastAction_); } actions_[i] = buttons[i]; }
        Touch(s, time); previous_ = s; if (layer_ != reportedLayer_) { reportedLayer_ = layer_; Publish(snapshot); }
    }
}
