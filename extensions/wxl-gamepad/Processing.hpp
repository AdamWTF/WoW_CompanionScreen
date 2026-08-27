#pragma once

#include <algorithm>
#include <cmath>

namespace wxl_gamepad
{
    struct Stick { float x{}, y{}; };

    inline float NormalizeSigned16(int value)
    {
        return value < 0 ? (std::max)(-1.0f, float(value) / 32768.0f)
                         : (std::min)(1.0f, float(value) / 32767.0f);
    }

    inline Stick RadialDeadzone(float x, float y, float deadzone)
    {
        const float length = std::sqrt(x * x + y * y);
        if (length <= deadzone || length <= 0.0f) return {};
        const float clamped = (std::min)(length, 1.0f);
        const float remapped = (clamped - deadzone) / (1.0f - deadzone);
        return { x / length * remapped, y / length * remapped };
    }

    inline bool Hysteresis(float value, bool active, float press, float release)
    {
        return active ? value > release : value >= press;
    }

    inline bool DirectionHysteresis(float value, bool active, float press, float release)
    {
        return active ? value > release : value >= press;
    }

    inline float Response(float value, float exponent)
    {
        return std::copysign(std::pow(std::abs(value), exponent), value);
    }
}
