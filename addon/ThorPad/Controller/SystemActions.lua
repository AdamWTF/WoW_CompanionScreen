local addonName, ThorPad = ...

ThorPad.SystemActions = {
    registry = {
        JUMP = {
            id = "JUMP",
            name = "Jump",
            icon = [[Interface\Icons\Ability_Rogue_Sprint]],
        },
    },
}

function ThorPad.SystemActions:Get(id) return self.registry[id] end
function ThorPad.SystemActions:All() return self.registry end
