#include "ControllerGameplay.hpp"
#include "ExtensionApi.hpp"
#include "Processing.hpp"

#include <windows.h>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

namespace wxl_gamepad { const WXL_Api* g_api = nullptr; }

namespace
{
    struct FakeInput final : wxl_gamepad::IGameInput
    {
        bool foreground{true}; bool movement[4]{}; int releases{}; float cameraX{}, cameraY{}; bool camera{}; std::vector<std::string> targets; std::vector<wxl_gamepad::GameCommand> commands;
        bool Foreground() const override { return foreground; }
        void Movement(wxl_gamepad::MovementControl control, bool down, uint32_t) override { movement[size_t(control)] = down; }
        void Target(const wxl_gamepad::KeyChord& chord) override { targets.push_back(chord.text); }
        void Command(wxl_gamepad::GameCommand command) override { commands.push_back(command); }
        void Camera(bool active, float dx, float dy) override { camera = active; cameraX += dx; cameraY += dy; }
        void PointerMove(int, int) override {} void PointerClick(bool) override {}
        void ReleaseAll(uint32_t) override { for (bool& value : movement) value = false; camera = false; ++releases; }
    };
}

int main()
{
    using namespace wxl_gamepad;
    assert(NormalizeSigned16(-32768) == -1.0f); assert(NormalizeSigned16(32767) == 1.0f); assert(NormalizeSigned16(0) == 0.0f);
    Stick dead = RadialDeadzone(.1f, .1f, .2f); assert(dead.x == 0 && dead.y == 0);
    Stick diagonal = RadialDeadzone(.5f, .5f, .2f); assert(diagonal.x > 0 && diagonal.y > 0 && std::abs(diagonal.x - diagonal.y) < .0001f);
    bool trigger = Hysteresis(.54f, false, .55f, .45f); assert(!trigger); trigger = Hysteresis(.56f, trigger, .55f, .45f); assert(trigger); trigger = Hysteresis(.50f, trigger, .55f, .45f); assert(trigger); trigger = Hysteresis(.44f, trigger, .55f, .45f); assert(!trigger);
    KeyChord chord; assert(ParseChord("CTRL+SHIFT+TAB", chord)); assert(chord.key == VK_TAB && chord.modifiers.size() == 2); assert(!ParseChord("CTRL+NO_SUCH_KEY", chord));
    char temp[MAX_PATH]{}; GetTempPathA(MAX_PATH, temp); std::string configPath = std::string(temp) + "wxl-gamepad-test.cfg";
    { std::ofstream file(configPath); file << "WXL_GAMEPAD_CAMERA_SENSITIVITY_X=2.0\n[Controller]\nBackend=SDL\nLeftStickDeadzone=0.30\n"; }
    ControllerConfig loaded = ControllerConfig::Load(configPath.c_str()); assert(loaded.backend == BackendKind::SDL); assert(std::abs(loaded.leftStickDeadzone - .30f) < .001f); assert(std::abs(loaded.cameraSensitivityX - 2.0f) < .001f);
    _putenv_s("WXL_GAMEPAD_BACKEND", "XInput"); loaded = ControllerConfig::Load(configPath.c_str()); assert(loaded.backend == BackendKind::XInput); _putenv_s("WXL_GAMEPAD_BACKEND", ""); DeleteFileA(configPath.c_str());
    assert(ControllerActionSlot(0, 0) == 1); assert(ControllerActionSlot(1, 4) == 49); assert(ControllerActionSlot(2, 7) == 60); assert(ControllerActionSlot(3, 7) == 68);

    ControllerConfig config; ParseChord("SHIFT+TAB", config.previousHostile); ParseChord("TAB", config.nextHostile); ParseChord("CTRL+TAB", config.nextFriendly);
    FakeInput input; ControllerGameplay gameplay(config, input, false); ControllerSnapshot snapshot; snapshot.generation = ~uint64_t{}; snapshot.connected = true;
    snapshot.state.leftY = -.8f; snapshot.state.leftX = .8f; gameplay.Update(snapshot, .01f, 10); assert(input.movement[0] && input.movement[3]);
    snapshot.state.leftY = snapshot.state.leftX = 0; gameplay.Update(snapshot, .01f, 20); assert(!input.movement[0] && !input.movement[3]);
    snapshot.state.leftTrigger = .56f; gameplay.Update(snapshot, .01f, 30); assert(gameplay.Layer() == 1); snapshot.state.rightTrigger = .56f; gameplay.Update(snapshot, .01f, 40); assert(gameplay.Layer() == 3);
    snapshot.state.leftTrigger = .44f; gameplay.Update(snapshot, .01f, 41); assert(gameplay.Layer() == 2); snapshot.state.rightTrigger = .44f; gameplay.Update(snapshot, .01f, 42); assert(gameplay.Layer() == 0);
    snapshot.state.leftShoulder = true; gameplay.Update(snapshot, .01f, 50); gameplay.Update(snapshot, .01f, 60); assert(input.targets.size() == 1 && input.targets[0] == "SHIFT+TAB");
    snapshot.state.start = true; snapshot.state.back = true; snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 61); gameplay.Update(snapshot, .01f, 62);
    assert(input.commands.size() == 3); assert(input.commands[0] == GameCommand::ToggleGameMenu); assert(input.commands[1] == GameCommand::ToggleAllBags); assert(input.commands[2] == GameCommand::NextView);
    snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = false; gameplay.Update(snapshot, .01f, 63); snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 64); assert(input.commands.size() == 6);
    snapshot.state.rightStickButton = true; gameplay.Update(snapshot, .01f, 65); assert(input.targets.size() == 2 && input.targets[1] == "CTRL+TAB");
    snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = snapshot.state.rightStickButton = false; gameplay.Update(snapshot, .01f, 66);
    input.foreground = false; snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 67); input.foreground = true; gameplay.Update(snapshot, .01f, 68); assert(input.commands.size() == 6);
    snapshot.connected = false; ++snapshot.generation; gameplay.Update(snapshot, .01f, 69); snapshot.connected = true; ++snapshot.generation; gameplay.Update(snapshot, .01f, 70); assert(input.commands.size() == 6);
    snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = false; gameplay.Update(snapshot, .01f, 71); snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 72); assert(input.commands.size() == 9);
    snapshot.state.rightX = 1; gameplay.Update(snapshot, .01f, 70); assert(input.camera && input.cameraX > 0); snapshot.state.rightX = 0; gameplay.Update(snapshot, .01f, 80); assert(!input.camera);
    gameplay.Release(90); assert(input.releases == 1);
    std::cout << "wxl-gamepad tests passed\n";
    return 0;
}
