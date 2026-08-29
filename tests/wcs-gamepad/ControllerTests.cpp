#include "ControllerGameplay.hpp"
#include "ExtensionApi.hpp"
#include "Processing.hpp"

#include <windows.h>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

namespace wcs_gamepad { const WXL_Api* g_api = nullptr; }

namespace
{
    struct FakeInput final : wcs_gamepad::IGameInput
    {
        struct SystemEvent { wcs_gamepad::CompanionSystemAction action; wcs_gamepad::InputState state; };
        bool foreground{true}; bool movement[4]{}; int releases{}; float cameraX{}, cameraY{}, pointerX{}, pointerY{}; bool camera{}; std::vector<std::string> targets; std::vector<wcs_gamepad::GameCommand> commands; std::vector<wcs_gamepad::UINavigationCommand> uiCommands; std::vector<int> wowActions; std::vector<SystemEvent> systemActions;
        bool Foreground() const override { return foreground; }
        void WoWAction(int slot) override { wowActions.push_back(slot); }
        void SystemAction(wcs_gamepad::CompanionSystemAction action, wcs_gamepad::InputState state, uint32_t) override { systemActions.push_back({action, state}); }
        void Movement(wcs_gamepad::MovementControl control, bool down, uint32_t) override { movement[size_t(control)] = down; }
        void Target(const wcs_gamepad::KeyChord& chord) override { targets.push_back(chord.text); }
        void Command(wcs_gamepad::GameCommand command) override { commands.push_back(command); }
        void Camera(bool active, float dx, float dy) override { camera = active; cameraX += dx; cameraY += dy; }
        void PointerMove(int, int) override {} void PointerClick(bool) override {}
        void MovePointerNormalized(float x, float y) override { pointerX = x; pointerY = y; }
        void UINavigation(wcs_gamepad::UINavigationCommand command) override { uiCommands.push_back(command); }
        void ReleaseAll(uint32_t) override { for (bool& value : movement) value = false; camera = false; ++releases; }
    };
}

int main()
{
    using namespace wcs_gamepad;
    ControllerConfig defaultSettings; assert(!defaultSettings.smartInteractDebug);
    assert(NormalizeSigned16(-32768) == -1.0f); assert(NormalizeSigned16(32767) == 1.0f); assert(NormalizeSigned16(0) == 0.0f);
    Stick dead = RadialDeadzone(.1f, .1f, .2f); assert(dead.x == 0 && dead.y == 0);
    Stick diagonal = RadialDeadzone(.5f, .5f, .2f); assert(diagonal.x > 0 && diagonal.y > 0 && std::abs(diagonal.x - diagonal.y) < .0001f);
    bool trigger = Hysteresis(.54f, false, .55f, .45f); assert(!trigger); trigger = Hysteresis(.56f, trigger, .55f, .45f); assert(trigger); trigger = Hysteresis(.50f, trigger, .55f, .45f); assert(trigger); trigger = Hysteresis(.44f, trigger, .55f, .45f); assert(!trigger);
    KeyChord chord; assert(ParseChord("CTRL+SHIFT+TAB", chord)); assert(chord.key == VK_TAB && chord.modifiers.size() == 2); assert(!ParseChord("CTRL+NO_SUCH_KEY", chord));
    char temp[MAX_PATH]{}; GetTempPathA(MAX_PATH, temp); std::string configPath = std::string(temp) + "wcs-gamepad-test.cfg";
    { std::ofstream file(configPath); file << "WCS_GAMEPAD_CAMERA_SENSITIVITY_X=2.0\n[Controller]\nBackend=SDL\nLeftStickDeadzone=0.30\nSmartInteractDebug=1\n"; }
    ControllerConfig loaded = ControllerConfig::Load(configPath.c_str()); assert(loaded.backend == BackendKind::SDL); assert(std::abs(loaded.leftStickDeadzone - .30f) < .001f); assert(std::abs(loaded.cameraSensitivityX - 2.0f) < .001f);
    assert(loaded.smartInteractDebug);
    _putenv_s("WCS_GAMEPAD_BACKEND", "XInput"); loaded = ControllerConfig::Load(configPath.c_str()); assert(loaded.backend == BackendKind::XInput); _putenv_s("WCS_GAMEPAD_BACKEND", ""); DeleteFileA(configPath.c_str());
    assert(ControllerActionSlot(0, 0) == 1); assert(ControllerActionSlot(1, 4) == 49); assert(ControllerActionSlot(2, 7) == 60); assert(ControllerActionSlot(3, 7) == 68);
    CompanionActionMap defaults; for (int layer = 0; layer < 4; ++layer) for (int control = 0; control < 8; ++control) { const CompanionAction& action = defaults.Get(layer, control); assert(action.type == CompanionActionType::WoWAction); assert(action.wowActionSlot == ControllerActionSlot(layer, control)); }
    CompanionActionMap interactLayers; const char* layerNames[] = {"default", "l2", "r2", "l2r2"};
    for (int layer = 0; layer < 4; ++layer) { assert(interactLayers.SetSystemAction(layerNames[layer], "east", "INTERACT")); assert(interactLayers.Get(layer, 5).systemAction == CompanionSystemAction::Interact); }
    interactLayers.Reset(); for (int layer = 0; layer < 4; ++layer) assert(interactLayers.Get(layer, 5).type == CompanionActionType::WoWAction);

    ControllerConfig config; ParseChord("SHIFT+TAB", config.previousHostile); ParseChord("TAB", config.nextHostile); ParseChord("CTRL+TAB", config.nextFriendly);
    FakeInput input; ControllerGameplay gameplay(config, input, false); ControllerSnapshot snapshot; snapshot.generation = ~uint64_t{}; snapshot.connected = true;
    snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = true; snapshot.state.leftY = -.8f; gameplay.Update(snapshot, .01f, 1); assert(input.commands.empty() && !input.movement[0]);
    gameplay.SetActive(true, 5); gameplay.Update(snapshot, .01f, 6); assert(input.commands.empty() && !input.movement[0]);
    snapshot.state = {}; gameplay.Update(snapshot, .01f, 7); assert(!input.movement[0]);
    snapshot.state.leftY = -.8f; snapshot.state.leftX = .8f; gameplay.Update(snapshot, .01f, 10); assert(input.movement[0] && input.movement[3]);
    snapshot.state.leftY = snapshot.state.leftX = 0; gameplay.Update(snapshot, .01f, 20); assert(!input.movement[0] && !input.movement[3]);
    snapshot.state.leftTrigger = .56f; gameplay.Update(snapshot, .01f, 30); assert(gameplay.Layer() == 1); snapshot.state.rightTrigger = .56f; gameplay.Update(snapshot, .01f, 40); assert(gameplay.Layer() == 3);
    snapshot.state.leftTrigger = .44f; gameplay.Update(snapshot, .01f, 41); assert(gameplay.Layer() == 2); snapshot.state.rightTrigger = .44f; gameplay.Update(snapshot, .01f, 42); assert(gameplay.Layer() == 0);
    snapshot.state.leftShoulder = true; gameplay.Update(snapshot, .01f, 50); gameplay.Update(snapshot, .01f, 60); assert(input.targets.size() == 1 && input.targets[0] == "SHIFT+TAB");
    snapshot.state.start = true; snapshot.state.back = true; snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 61); gameplay.Update(snapshot, .01f, 62);
    assert(input.commands.size() == 2); assert(input.commands[0] == GameCommand::ToggleGameMenu); assert(input.commands[1] == GameCommand::ToggleAllBags);
    snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = false; gameplay.Update(snapshot, .01f, 63); snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 64); assert(input.commands.size() == 4);
    snapshot.state.rightStickButton = true; gameplay.Update(snapshot, .01f, 65); assert(input.targets.size() == 2 && input.targets[1] == "CTRL+TAB");
    snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = snapshot.state.rightStickButton = false; gameplay.Update(snapshot, .01f, 66);
    input.foreground = false; snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 67); input.foreground = true; gameplay.Update(snapshot, .01f, 68); assert(input.commands.size() == 4);
    snapshot.connected = false; ++snapshot.generation; gameplay.Update(snapshot, .01f, 69); snapshot.connected = true; ++snapshot.generation; gameplay.Update(snapshot, .01f, 70); assert(input.commands.size() == 4);
    snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = false; gameplay.Update(snapshot, .01f, 71); snapshot.state.start = snapshot.state.back = snapshot.state.leftStickButton = true; gameplay.Update(snapshot, .01f, 72); assert(input.commands.size() == 6);
    snapshot.state.rightX = 1; gameplay.Update(snapshot, .01f, 70); assert(input.camera && input.cameraX > 0); snapshot.state.rightX = 0; gameplay.Update(snapshot, .01f, 80); assert(!input.camera);
    gameplay.SetActive(false, 90); assert(!gameplay.Active() && input.releases == 5); const size_t commandCount = input.commands.size(); snapshot.state.start = true; gameplay.Update(snapshot, .01f, 91); assert(input.commands.size() == commandCount);

    FakeInput uiInput; ControllerGameplay uiGameplay(config, uiInput, false); ControllerSnapshot uiSnapshot; uiSnapshot.generation = ~uint64_t{}; uiSnapshot.connected = true;
    uiGameplay.SetActive(true, 300); uiGameplay.Update(uiSnapshot, .01f, 301); assert(!uiGameplay.UINavigationActive());
    assert(uiGameplay.SetSystemAction("default", "south", "JUMP"));
    uiSnapshot.state.leftY = -.8f; uiSnapshot.state.south = true; uiGameplay.Update(uiSnapshot, .01f, 302);
    assert(uiInput.movement[0] && uiInput.systemActions.size() == 1 && uiInput.systemActions.back().state == InputState::Pressed);
    uiGameplay.SetUINavigationActive(true, 303); assert(uiGameplay.UINavigationActive() && !uiInput.movement[0]);
    assert(uiInput.systemActions.size() == 2 && uiInput.systemActions.back().state == InputState::Released);
    uiSnapshot.state.dpadUp = true; uiGameplay.Update(uiSnapshot, .01f, 304); assert(uiInput.uiCommands.empty());
    uiSnapshot.state = {}; uiGameplay.Update(uiSnapshot, .01f, 305); assert(uiInput.uiCommands.empty());
    uiSnapshot.state.dpadUp = true; uiGameplay.Update(uiSnapshot, .01f, 306); assert(uiInput.uiCommands.size() == 1 && uiInput.uiCommands.back() == UINavigationCommand::Up);
    uiGameplay.Update(uiSnapshot, .01f, 655); assert(uiInput.uiCommands.size() == 1); uiGameplay.Update(uiSnapshot, .01f, 656); assert(uiInput.uiCommands.size() == 2 && uiInput.uiCommands.back() == UINavigationCommand::Up);
    uiGameplay.Update(uiSnapshot, .01f, 756); assert(uiInput.uiCommands.size() == 3);
    uiSnapshot.state.dpadDown = true; uiGameplay.Update(uiSnapshot, .01f, 757); assert(uiInput.uiCommands.size() == 3);
    uiSnapshot.state.dpadUp = uiSnapshot.state.dpadDown = false; uiSnapshot.state.south = true; uiGameplay.Update(uiSnapshot, .01f, 758); assert(uiInput.uiCommands.back() == UINavigationCommand::Confirm);
    uiSnapshot.state.south = false; uiSnapshot.state.east = true; uiGameplay.Update(uiSnapshot, .01f, 759); assert(uiInput.uiCommands.back() == UINavigationCommand::Back);
    const size_t uiActionCount = uiInput.wowActions.size(), uiTargetCount = uiInput.targets.size(); uiSnapshot.state.leftShoulder = true; uiSnapshot.state.leftY = -.8f; uiSnapshot.state.rightX = 1; uiGameplay.Update(uiSnapshot, .01f, 759);
    assert(uiInput.wowActions.size() == uiActionCount && uiInput.targets.size() == uiTargetCount && !uiInput.movement[0] && !uiInput.camera);
    uiSnapshot.state.start = true; uiSnapshot.state.back = true; uiGameplay.Update(uiSnapshot, .01f, 760); assert(uiInput.commands.size() == 2 && uiInput.commands[0] == GameCommand::ToggleGameMenu && uiInput.commands[1] == GameCommand::ToggleAllBags);
    const size_t uiBeforeFocusLoss = uiInput.uiCommands.size(); uiInput.foreground = false; uiSnapshot.state.dpadRight = true; uiGameplay.Update(uiSnapshot, .01f, 761); uiInput.foreground = true; uiGameplay.Update(uiSnapshot, .01f, 762); assert(uiInput.uiCommands.size() == uiBeforeFocusLoss);
    uiSnapshot.state = {}; uiGameplay.Update(uiSnapshot, .01f, 763); uiSnapshot.state.dpadRight = true; uiGameplay.Update(uiSnapshot, .01f, 764); assert(uiInput.uiCommands.back() == UINavigationCommand::Right);
    const size_t uiBeforeDisconnect = uiInput.uiCommands.size(); uiSnapshot.connected = false; ++uiSnapshot.generation; uiGameplay.Update(uiSnapshot, .01f, 765); uiSnapshot.connected = true; ++uiSnapshot.generation; uiGameplay.Update(uiSnapshot, .01f, 766); assert(uiInput.uiCommands.size() == uiBeforeDisconnect);
    uiSnapshot.state = {}; uiGameplay.Update(uiSnapshot, .01f, 767); uiGameplay.SetUINavigationActive(false, 768); assert(!uiGameplay.UINavigationActive()); uiGameplay.Update(uiSnapshot, .01f, 769);
    uiSnapshot.state.south = true; uiGameplay.Update(uiSnapshot, .01f, 770); assert(uiInput.systemActions.back().state == InputState::Pressed);

    FakeInput actionInput; ControllerGameplay actionGameplay(config, actionInput, false); ControllerSnapshot actionSnapshot; actionSnapshot.generation = 1; actionSnapshot.connected = true;
    actionGameplay.SetActive(true, 100); actionGameplay.Update(actionSnapshot, .01f, 101);
    assert(actionGameplay.SetSystemAction("l2", "south", "JUMP")); assert(ControllerGameplay::SupportsSystemAction("JUMP")); assert(ControllerGameplay::SupportsSystemAction("INTERACT")); assert(!ControllerGameplay::SupportsSystemAction("AUTO_RUN"));
    actionSnapshot.state.leftTrigger = .8f; actionSnapshot.state.south = true; actionGameplay.Update(actionSnapshot, .01f, 102);
    assert(actionInput.systemActions.size() == 1 && actionInput.systemActions[0].state == InputState::Pressed);
    actionSnapshot.state.leftTrigger = 0; actionGameplay.Update(actionSnapshot, .01f, 103); assert(actionInput.systemActions.size() == 1);
    actionSnapshot.state.south = false; actionGameplay.Update(actionSnapshot, .01f, 104);
    assert(actionInput.systemActions.size() == 2 && actionInput.systemActions[1].state == InputState::Released);

    assert(actionGameplay.SetSystemAction("default", "south", "JUMP")); assert(actionGameplay.SetSystemAction("default", "east", "JUMP"));
    actionSnapshot.state.south = true; actionGameplay.Update(actionSnapshot, .01f, 105); actionSnapshot.state.east = true; actionGameplay.Update(actionSnapshot, .01f, 106);
    assert(actionInput.systemActions.size() == 3);
    actionSnapshot.state.south = false; actionGameplay.Update(actionSnapshot, .01f, 107); assert(actionInput.systemActions.size() == 3);
    actionSnapshot.state.east = false; actionGameplay.Update(actionSnapshot, .01f, 108); assert(actionInput.systemActions.size() == 4 && actionInput.systemActions.back().state == InputState::Released);

    assert(actionGameplay.SetSystemAction("default", "east", "INTERACT"));
    actionSnapshot.state.east = true; actionGameplay.Update(actionSnapshot, .01f, 108);
    assert(actionInput.systemActions.size() == 5 && actionInput.systemActions.back().action == CompanionSystemAction::Interact && actionInput.systemActions.back().state == InputState::Pressed);
    actionSnapshot.state.east = false; actionGameplay.Update(actionSnapshot, .01f, 108);
    assert(actionInput.systemActions.size() == 5); // INTERACT is intentionally press-only.

    assert(!actionGameplay.SetSystemAction("default", "west", "FUTURE_ACTION")); actionSnapshot.state.west = true; actionGameplay.Update(actionSnapshot, .01f, 109); actionSnapshot.state.west = false; actionGameplay.Update(actionSnapshot, .01f, 110);
    assert(actionInput.systemActions.size() == 5 && actionInput.wowActions.empty());
    actionGameplay.ResetSystemActions(111); actionSnapshot.state.north = true; actionGameplay.Update(actionSnapshot, .01f, 112); assert(actionInput.wowActions.size() == 1 && actionInput.wowActions[0] == 8);
    actionSnapshot.state.north = false; actionGameplay.Update(actionSnapshot, .01f, 113); assert(actionInput.wowActions.size() == 1);
    assert(actionGameplay.SetSystemAction("default", "south", "JUMP")); actionSnapshot.state.south = true; actionGameplay.Update(actionSnapshot, .01f, 114); const size_t beforeReset = actionInput.systemActions.size(); actionGameplay.ResetSystemActions(115); assert(actionInput.systemActions.size() == beforeReset + 1 && actionInput.systemActions.back().state == InputState::Released);
    actionSnapshot.state.south = false; actionGameplay.Update(actionSnapshot, .01f, 116); assert(actionGameplay.SetSystemAction("default", "south", "JUMP"));
    actionSnapshot.state.south = true; actionGameplay.Update(actionSnapshot, .01f, 117); const size_t beforeFocusLoss = actionInput.systemActions.size(); actionInput.foreground = false; actionGameplay.Update(actionSnapshot, .01f, 118); assert(actionInput.systemActions.size() == beforeFocusLoss + 1 && actionInput.systemActions.back().state == InputState::Released);
    actionInput.foreground = true; actionGameplay.Update(actionSnapshot, .01f, 119); actionSnapshot.state.south = false; actionGameplay.Update(actionSnapshot, .01f, 120);
    actionSnapshot.state.south = true; actionGameplay.Update(actionSnapshot, .01f, 121); const size_t beforeDisconnect = actionInput.systemActions.size(); actionSnapshot.connected = false; ++actionSnapshot.generation; actionGameplay.Update(actionSnapshot, .01f, 122); assert(actionInput.systemActions.size() == beforeDisconnect + 1 && actionInput.systemActions.back().state == InputState::Released);
    actionSnapshot.connected = true; ++actionSnapshot.generation; actionGameplay.Update(actionSnapshot, .01f, 123); actionSnapshot.state.south = false; actionGameplay.Update(actionSnapshot, .01f, 124); actionSnapshot.state.south = true; actionGameplay.Update(actionSnapshot, .01f, 125); const size_t beforeDeactivate = actionInput.systemActions.size(); actionGameplay.SetActive(false, 126); assert(actionInput.systemActions.size() == beforeDeactivate + 1 && actionInput.systemActions.back().state == InputState::Released);

    FakeInput cleanupInput; ControllerGameplay cleanupGameplay(config, cleanupInput, false); ControllerSnapshot cleanupSnapshot; cleanupSnapshot.generation = 1; cleanupSnapshot.connected = true;
    cleanupGameplay.SetActive(true, 200); cleanupGameplay.Update(cleanupSnapshot, .01f, 201); assert(cleanupGameplay.SetSystemAction("default", "east", "INTERACT"));
    cleanupSnapshot.state.east = true; cleanupGameplay.Update(cleanupSnapshot, .01f, 202); assert(cleanupInput.systemActions.size() == 1);
    cleanupInput.foreground = false; cleanupGameplay.Update(cleanupSnapshot, .01f, 203); assert(cleanupInput.systemActions.size() == 1);
    cleanupInput.foreground = true; cleanupGameplay.Update(cleanupSnapshot, .01f, 204); cleanupSnapshot.state.east = false; cleanupGameplay.Update(cleanupSnapshot, .01f, 205);
    cleanupSnapshot.state.east = true; cleanupGameplay.Update(cleanupSnapshot, .01f, 206); assert(cleanupInput.systemActions.size() == 2);
    cleanupSnapshot.connected = false; ++cleanupSnapshot.generation; cleanupGameplay.Update(cleanupSnapshot, .01f, 207); assert(cleanupInput.systemActions.size() == 2);
    cleanupSnapshot.connected = true; ++cleanupSnapshot.generation; cleanupGameplay.Update(cleanupSnapshot, .01f, 208); cleanupSnapshot.state.east = false; cleanupGameplay.Update(cleanupSnapshot, .01f, 209);
    cleanupSnapshot.state.east = true; cleanupGameplay.Update(cleanupSnapshot, .01f, 210); assert(cleanupInput.systemActions.size() == 3);
    cleanupGameplay.SetActive(false, 211); assert(cleanupInput.systemActions.size() == 3);
    std::cout << "wcs-gamepad tests passed\n";
    return 0;
}
