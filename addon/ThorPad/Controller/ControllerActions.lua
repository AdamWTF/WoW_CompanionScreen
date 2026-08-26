local addonName, ThorPad = ...
local C = ThorPad.Constants

ThorPad.Controller = { buttons = {}, pending = false }
local Controller = ThorPad.Controller

function Controller:ButtonName(layer, control)
    local prefix = C.CONTROLLER_BUTTON_PREFIXES[layer]
    for index, candidate in ipairs(C.CONTROLLER_CONTROLS) do if candidate == control then return prefix and (prefix .. index) end end
end

function Controller:GetActionID(layer, control)
    local actions = C.CONTROLLER_ACTIONS[layer]
    if not actions then return nil end
    for index, candidate in ipairs(C.CONTROLLER_CONTROLS) do if candidate == control then return actions[index] end end
end

function Controller:CreateButtons()
    for _, layer in ipairs(C.CONTROLLER_LAYERS) do
        self.buttons[layer] = {}
        for index, control in ipairs(C.CONTROLLER_CONTROLS) do
            local button = CreateFrame("Button", C.CONTROLLER_BUTTON_PREFIXES[layer] .. index, UIParent, "SecureActionButtonTemplate")
            button:SetSize(1, 1); button:SetPoint("BOTTOMLEFT", UIParent, "BOTTOMLEFT", -4, -4); button:SetAlpha(0); button:EnableMouse(false); button:Show()
            button.layer, button.control, button.actionID = layer, control, C.CONTROLLER_ACTIONS[layer][index]
            self.buttons[layer][control] = button
        end
    end
    self:ApplyAll()
end


function Controller:Apply(button)
    if InCombatLockdown() then self.pending = true; return false end
    button:SetAttribute("type", "action"); button:SetAttribute("action", button.actionID)
    return true
end

function Controller:ApplyAll()
    if InCombatLockdown() then self.pending = true; return end
    self.pending = false
    for _, layer in ipairs(C.CONTROLLER_LAYERS) do for _, control in ipairs(C.CONTROLLER_CONTROLS) do self:Apply(self.buttons[layer][control]) end end
end

function Controller:GetButton(layer, control)
    local layerKey = string.lower(layer or ""):gsub("%s", ""):gsub("%+", "")
    local controlKey = string.lower(control or ""):gsub("%s", "_"):gsub("%-", "_")
    controlKey = ({ dpadup = "dpad_up", dpaddown = "dpad_down", dpadleft = "dpad_left", dpadright = "dpad_right" })[controlKey] or controlKey
    return self.buttons[layerKey] and self.buttons[layerKey][controlKey] or nil
end

function Controller:GetAssignment(layer, control)
    local actionID = self:GetActionID(layer, control)
    return actionID and { type = "action", actionID = actionID } or nil
end

function Controller:IsEnabled() return ThorPadDB.controller.enabled end

function Controller:TakeAssignment(layer, control)
    if InCombatLockdown() then return false, "combat" end
    local actionID = self:GetActionID(layer, control)
    if not actionID or not HasAction(actionID) then return false, "empty" end
    PickupAction(actionID); return true
end

function Controller:DropOn(layer, control)
    if InCombatLockdown() then return false, "combat" end
    local actionID = self:GetActionID(layer, control)
    if not actionID or not GetCursorInfo() then return false, "unsupported" end
    PlaceAction(actionID); return true
end

function Controller:ClearAssignment(layer, control)
    if InCombatLockdown() then return false, "combat" end
    local actionID = self:GetActionID(layer, control)
    if not actionID or not HasAction(actionID) then return false, "empty" end
    PickupAction(actionID); ClearCursor(); return true
end

function Controller:ClearInternalCursor() end
function Controller:CursorChanged() end

function Controller:GetState(descriptor)
    local actionID = descriptor and descriptor.actionID
    if not actionID then return nil end
    local usable, noResource = IsUsableAction(actionID); local start, duration, enabled = GetActionCooldown(actionID)
    return {
        icon = GetActionTexture(actionID), count = GetActionCount(actionID), usable = usable, noResource = noResource,
        start = start or 0, duration = duration or 0, enabled = enabled or 0, current = IsCurrentAction(actionID), inRange = IsActionInRange(actionID),
    }
end
