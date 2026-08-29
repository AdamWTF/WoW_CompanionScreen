local addonName, WCS = ...

WCS.SystemActions = {
    registry = {
        JUMP = {
            id = "JUMP",
            name = "Jump",
            icon = [[Interface\Icons\Ability_Rogue_Sprint]],
        },
        INTERACT = {
            id = "INTERACT",
            name = "Interact",
            icon = [[Interface\Cursor\Interact]],
        },
    },
    order = { "JUMP", "INTERACT" },
}

function WCS.SystemActions:Get(id) return self.registry[id] end
function WCS.SystemActions:All()
    local actions = {}
    for _, id in ipairs(self.order) do actions[#actions + 1] = self.registry[id] end
    return actions
end
