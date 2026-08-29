#pragma once

#include "ControllerTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace wcs_gamepad
{
    struct KeyChord
    {
        std::vector<unsigned> modifiers;
        unsigned key{};
        std::string text;
    };

    struct ControllerConfig
    {
        bool enabled{true}, invertCameraY{}, debug{}, smartInteractDebug{};
        BackendKind backend{BackendKind::Auto};
        std::string glyphStyle{"Auto"};
        float leftStickDeadzone{.20f}, rightStickDeadzone{.15f};
        float triggerPressThreshold{.55f}, triggerReleaseThreshold{.45f};
        float cameraSensitivityX{1.0f}, cameraSensitivityY{1.0f}, cameraResponseCurve{1.5f};
        float movementPressThreshold{.35f}, movementReleaseThreshold{.25f}, cameraMaxPixelsPerSecond{900.0f};
        int pollingRateHz{125};
        KeyChord previousHostile, nextHostile, nextFriendly;

        static ControllerConfig Load(const char* path);
    };

    bool ParseChord(const std::string& text, KeyChord& chord);
}
