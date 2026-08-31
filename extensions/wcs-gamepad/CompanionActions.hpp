#pragma once

#include <array>
#include <string_view>

namespace wcs_gamepad
{
    enum class CompanionActionType { None, WoWAction, SystemAction };
    enum class CompanionSystemAction { Unknown, Jump, Interact };
    enum class InputState { Pressed, Released };

    struct CompanionAction
    {
        CompanionActionType type{CompanionActionType::None};
        int wowActionSlot{};
        CompanionSystemAction systemAction{CompanionSystemAction::Unknown};

        static CompanionAction WoW(int slot) { return {CompanionActionType::WoWAction, slot, CompanionSystemAction::Unknown}; }
        static CompanionAction System(CompanionSystemAction action) { return {CompanionActionType::SystemAction, 0, action}; }
    };

    inline int ControllerActionSlot(int layer, int control)
    {
        static constexpr int slots[4][8] = {{1,2,3,4,5,6,7,8}, {9,10,11,12,49,50,51,52}, {53,54,55,56,57,58,59,60}, {61,62,63,64,65,66,67,68}};
        return layer >= 0 && layer < 4 && control >= 0 && control < 8 ? slots[layer][control] : 0;
    }

    inline int ControllerLayer(std::string_view layer)
    {
        if (layer == "default") return 0;
        if (layer == "l2") return 1;
        if (layer == "r2") return 2;
        if (layer == "l2r2") return 3;
        return -1;
    }

    inline int ControllerControl(std::string_view control)
    {
        static constexpr std::string_view controls[] = {"dpad_up", "dpad_down", "dpad_left", "dpad_right", "south", "east", "west", "north"};
        for (int index = 0; index < 8; ++index) if (control == controls[index]) return index;
        return -1;
    }

    inline CompanionSystemAction ParseSystemAction(std::string_view action)
    {
        if (action == "JUMP") return CompanionSystemAction::Jump;
        if (action == "INTERACT") return CompanionSystemAction::Interact;
        return CompanionSystemAction::Unknown;
    }

    inline const char* SystemActionName(CompanionSystemAction action)
    {
        if (action == CompanionSystemAction::Jump) return "JUMP";
        if (action == CompanionSystemAction::Interact) return "INTERACT";
        return "UNKNOWN";
    }

    class CompanionActionMap final
    {
    public:
        CompanionActionMap() { Reset(); }

        void Reset()
        {
            for (int layer = 0; layer < 4; ++layer)
                for (int control = 0; control < 8; ++control)
                    actions_[layer][control] = CompanionAction::WoW(ControllerActionSlot(layer, control));
        }

        bool SetSystemAction(std::string_view layer, std::string_view control, std::string_view action)
        {
            const int layerIndex = ControllerLayer(layer), controlIndex = ControllerControl(control);
            if (layerIndex < 0 || controlIndex < 0) return false;
            const CompanionSystemAction parsed = ParseSystemAction(action);
            actions_[layerIndex][controlIndex] = CompanionAction::System(parsed);
            return parsed != CompanionSystemAction::Unknown;
        }

        bool SetWoWAction(std::string_view layer, std::string_view control, int slot)
        {
            const int layerIndex = ControllerLayer(layer), controlIndex = ControllerControl(control);
            if (layerIndex < 0 || controlIndex < 0 || slot < 1 || slot > 120) return false;
            actions_[layerIndex][controlIndex] = CompanionAction::WoW(slot);
            return true;
        }

        const CompanionAction& Get(int layer, int control) const
        {
            static const CompanionAction none;
            return layer >= 0 && layer < 4 && control >= 0 && control < 8 ? actions_[layer][control] : none;
        }

    private:
        std::array<std::array<CompanionAction, 8>, 4> actions_{};
    };
}
