#include "ControllerConfig.hpp"
#include "ExtensionApi.hpp"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <unordered_map>

namespace wcs_gamepad
{
    namespace
    {
        using Values = std::unordered_map<std::string, std::string>;

        std::string Trim(std::string value)
        {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
            return value;
        }
        std::string Upper(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return char(std::toupper(c)); });
            return value;
        }
        Values ReadFile(const char* path)
        {
            Values values; FILE* file = nullptr;
            if (fopen_s(&file, path, "rb") != 0 || !file) return values;
            bool controller = false; char line[512];
            while (fgets(line, sizeof line, file))
            {
                std::string text = Trim(line);
                if (text.empty() || text[0] == '#' || text[0] == ';') continue;
                if (text.front() == '[' && text.back() == ']') { controller = Upper(Trim(text.substr(1, text.size() - 2))) == "CONTROLLER"; continue; }
                const size_t equals = text.find('='); if (equals == std::string::npos) continue;
                std::string key = Trim(text.substr(0, equals)), value = Trim(text.substr(equals + 1));
                values[(controller ? "SECTION:" : "FLAT:") + Upper(key)] = value;
            }
            fclose(file); return values;
        }
        std::string Resolve(const Values& values, const char* canonical, const char* alias, const char* fallback)
        {
            char env[256]{}; size_t count = 0;
            if (getenv_s(&count, env, sizeof env, alias) == 0 && count > 0) return env;
            auto it = values.find("SECTION:" + Upper(canonical)); if (it != values.end()) return it->second;
            it = values.find("FLAT:" + Upper(alias)); return it != values.end() ? it->second : fallback;
        }
        bool BoolValue(const std::string& raw, bool fallback, const char* key)
        {
            const std::string value = Upper(raw);
            if (value == "1" || value == "TRUE" || value == "YES" || value == "ON") return true;
            if (value == "0" || value == "FALSE" || value == "NO" || value == "OFF") return false;
            g_api->Log(WXL_LOG_WARN, kTag, "invalid %s='%s'; using default", key, raw.c_str()); return fallback;
        }
        float FloatValue(const std::string& raw, float fallback, float minimum, float maximum, const char* key)
        {
            char* end = nullptr; const float value = std::strtof(raw.c_str(), &end);
            if (end && !*end && value >= minimum && value <= maximum) return value;
            g_api->Log(WXL_LOG_WARN, kTag, "invalid %s='%s'; using default", key, raw.c_str()); return fallback;
        }
        int IntValue(const std::string& raw, int fallback, int minimum, int maximum, const char* key)
        {
            char* end = nullptr; const long value = std::strtol(raw.c_str(), &end, 10);
            if (end && !*end && value >= minimum && value <= maximum) return int(value);
            g_api->Log(WXL_LOG_WARN, kTag, "invalid %s='%s'; using default", key, raw.c_str()); return fallback;
        }
        std::string ConfigValue(const Values& values, const char* key, const char* fallback)
        {
            std::string snake;
            for (const char c : std::string(key))
            {
                if (std::isupper(static_cast<unsigned char>(c)) && !snake.empty()) snake.push_back('_');
                snake.push_back(char(std::toupper(static_cast<unsigned char>(c))));
            }
            std::string alias = "WCS_GAMEPAD_" + snake; return Resolve(values, key, alias.c_str(), fallback);
        }
    }

    bool ParseChord(const std::string& text, KeyChord& chord)
    {
        static const std::map<std::string, unsigned> named = {
            {"TAB", VK_TAB}, {"SPACE", VK_SPACE}, {"ENTER", VK_RETURN}, {"ESCAPE", VK_ESCAPE},
            {"BACKSPACE", VK_BACK}, {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT},
            {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4}, {"F5", VK_F5}, {"F6", VK_F6},
            {"F7", VK_F7}, {"F8", VK_F8}, {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},
        };
        KeyChord parsed; parsed.text = Upper(Trim(text)); size_t start = 0;
        while (start <= parsed.text.size())
        {
            const size_t plus = parsed.text.find('+', start);
            const std::string token = Trim(parsed.text.substr(start, plus == std::string::npos ? std::string::npos : plus - start));
            if (token == "SHIFT") parsed.modifiers.push_back(VK_SHIFT);
            else if (token == "CTRL" || token == "CONTROL") parsed.modifiers.push_back(VK_CONTROL);
            else if (token == "ALT") parsed.modifiers.push_back(VK_MENU);
            else if (token.size() == 1)
            {
                const SHORT value = VkKeyScanA(token[0]); if (value == -1 || parsed.key) return false; parsed.key = LOBYTE(value);
            }
            else { const auto it = named.find(token); if (it == named.end() || parsed.key) return false; parsed.key = it->second; }
            if (plus == std::string::npos) break; start = plus + 1;
        }
        if (!parsed.key) return false; chord = std::move(parsed); return true;
    }

    ControllerConfig ControllerConfig::Load(const char* path)
    {
        const Values values = ReadFile(path); ControllerConfig config;
        auto value = [&](const char* key, const char* fallback) { return ConfigValue(values, key, fallback); };
        config.enabled = BoolValue(value("Enabled", "1"), true, "Enabled");
        const std::string backend = Upper(value("Backend", "Auto"));
        if (backend == "AUTO") config.backend = BackendKind::Auto; else if (backend == "XINPUT") config.backend = BackendKind::XInput;
        else if (backend == "SDL") config.backend = BackendKind::SDL; else if (backend == "SDLJOYSTICK") config.backend = BackendKind::SDLJoystick;
        else g_api->Log(WXL_LOG_WARN, kTag, "invalid Backend='%s'; using Auto", backend.c_str());
        const std::string glyph = Upper(value("GlyphStyle", "Auto"));
        if (glyph == "AUTO") config.glyphStyle = "Auto"; else if (glyph == "PLAYSTATION") config.glyphStyle = "PlayStation";
        else if (glyph == "XBOX") config.glyphStyle = "Xbox"; else if (glyph == "THOR") config.glyphStyle = "Thor";
        else g_api->Log(WXL_LOG_WARN, kTag, "invalid GlyphStyle='%s'; using Auto", glyph.c_str());
        config.leftStickDeadzone = FloatValue(value("LeftStickDeadzone", ".20"), .20f, 0, .95f, "LeftStickDeadzone");
        config.rightStickDeadzone = FloatValue(value("RightStickDeadzone", ".15"), .15f, 0, .95f, "RightStickDeadzone");
        config.triggerPressThreshold = FloatValue(value("TriggerPressThreshold", ".55"), .55f, 0, 1, "TriggerPressThreshold");
        config.triggerReleaseThreshold = FloatValue(value("TriggerReleaseThreshold", ".45"), .45f, 0, 1, "TriggerReleaseThreshold");
        if (config.triggerReleaseThreshold >= config.triggerPressThreshold) { config.triggerPressThreshold = .55f; config.triggerReleaseThreshold = .45f; Log(WXL_LOG_WARN, "trigger release threshold must be below press threshold; using defaults"); }
        config.cameraSensitivityX = FloatValue(value("CameraSensitivityX", "1"), 1, 0, 10, "CameraSensitivityX");
        config.cameraSensitivityY = FloatValue(value("CameraSensitivityY", "1"), 1, 0, 10, "CameraSensitivityY");
        config.cameraResponseCurve = FloatValue(value("CameraResponseCurve", "1.5"), 1.5f, .1f, 5, "CameraResponseCurve");
        config.invertCameraY = BoolValue(value("InvertCameraY", "0"), false, "InvertCameraY");
        config.debug = BoolValue(value("ControllerDebug", "0"), false, "ControllerDebug");
        config.smartInteractDebug = BoolValue(value("SmartInteractDebug", "0"), false, "SmartInteractDebug");
        config.movementPressThreshold = FloatValue(value("MovementPressThreshold", ".35"), .35f, 0, 1, "MovementPressThreshold");
        config.movementReleaseThreshold = FloatValue(value("MovementReleaseThreshold", ".25"), .25f, 0, 1, "MovementReleaseThreshold");
        config.cameraMaxPixelsPerSecond = FloatValue(value("CameraMaxPixelsPerSecond", "900"), 900, 1, 5000, "CameraMaxPixelsPerSecond");
        config.pollingRateHz = IntValue(value("PollingRateHz", "125"), 125, 20, 1000, "PollingRateHz");
        const struct { const char* key; const char* fallback; KeyChord* target; } chords[] = {
            {"PreviousHostileChord", "SHIFT+TAB", &config.previousHostile}, {"NextHostileChord", "TAB", &config.nextHostile},
            {"NextFriendlyChord", "CTRL+TAB", &config.nextFriendly},
        };
        for (const auto& item : chords) if (!ParseChord(value(item.key, item.fallback), *item.target)) { ParseChord(item.fallback, *item.target); g_api->Log(WXL_LOG_WARN, kTag, "invalid %s; using %s", item.key, item.fallback); }
        return config;
    }
}
