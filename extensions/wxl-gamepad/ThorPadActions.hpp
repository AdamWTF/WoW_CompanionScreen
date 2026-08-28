#pragma once

#include <array>
#include <string_view>

namespace wxl_gamepad
{
    enum class ThorPadActionType { None, WoWAction, SystemAction };
    enum class ThorPadSystemAction { Unknown, Jump };
    enum class InputState { Pressed, Released };

    struct ThorPadAction
    {
        ThorPadActionType type{ThorPadActionType::None};
        int wowActionSlot{};
        ThorPadSystemAction systemAction{ThorPadSystemAction::Unknown};

        static ThorPadAction WoW(int slot) { return {ThorPadActionType::WoWAction, slot, ThorPadSystemAction::Unknown}; }
        static ThorPadAction System(ThorPadSystemAction action) { return {ThorPadActionType::SystemAction, 0, action}; }
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

    inline ThorPadSystemAction ParseSystemAction(std::string_view action)
    {
        return action == "JUMP" ? ThorPadSystemAction::Jump : ThorPadSystemAction::Unknown;
    }

    inline const char* SystemActionName(ThorPadSystemAction action)
    {
        return action == ThorPadSystemAction::Jump ? "JUMP" : "UNKNOWN";
    }

    class ThorPadActionMap final
    {
    public:
        ThorPadActionMap() { Reset(); }

        void Reset()
        {
            for (int layer = 0; layer < 4; ++layer)
                for (int control = 0; control < 8; ++control)
                    actions_[layer][control] = ThorPadAction::WoW(ControllerActionSlot(layer, control));
        }

        bool SetSystemAction(std::string_view layer, std::string_view control, std::string_view action)
        {
            const int layerIndex = ControllerLayer(layer), controlIndex = ControllerControl(control);
            if (layerIndex < 0 || controlIndex < 0) return false;
            const ThorPadSystemAction parsed = ParseSystemAction(action);
            actions_[layerIndex][controlIndex] = ThorPadAction::System(parsed);
            return parsed != ThorPadSystemAction::Unknown;
        }

        const ThorPadAction& Get(int layer, int control) const
        {
            static const ThorPadAction none;
            return layer >= 0 && layer < 4 && control >= 0 && control < 8 ? actions_[layer][control] : none;
        }

    private:
        std::array<std::array<ThorPadAction, 8>, 4> actions_{};
    };
}
