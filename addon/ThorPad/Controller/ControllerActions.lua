local addonName, ThorPad = ...
local C = ThorPad.Constants

ThorPad.Controller = { buttons = {}, pending = false, syncDirty = true, unknownLogged = {} }
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

function Controller:GetOverride(layer, control)
    local assignments = ThorPadDB and ThorPadDB.controller and ThorPadDB.controller.assignments
    local assignment = assignments and assignments[layer] and assignments[layer][control]
    return type(assignment) == "table" and assignment.type == "system" and type(assignment.action) == "string" and assignment or nil
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
    self:ApplyAll(); self:MarkSyncDirty()
end

function Controller:Apply(button)
    if InCombatLockdown() then self.pending = true; return false end
    if self:GetOverride(button.layer, button.control) then button:SetAttribute("type", nil); button:SetAttribute("action", nil)
    else button:SetAttribute("type", "action"); button:SetAttribute("action", button.actionID) end
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
    local override = self:GetOverride(layer, control)
    if override then
        local metadata = ThorPad.SystemActions:Get(override.action)
        return { type = "system", systemAction = override.action, name = metadata and metadata.name or ("Unknown: " .. override.action), icon = metadata and metadata.icon or [[Interface\Icons\INV_Misc_QuestionMark]], unknown = metadata == nil }
    end
    local actionID = self:GetActionID(layer, control)
    return actionID and { type = "action", actionID = actionID } or nil
end

function Controller:IsEnabled() return ThorPadDB.controller.enabled end

function Controller:EnsureCursorFrame()
    if self.cursorFrame then return end
    local frame = CreateFrame("Frame", "ThorPadSystemActionCursor", UIParent); frame:SetSize(42, 42); frame:SetFrameStrata("TOOLTIP"); frame:EnableMouse(false); frame:Hide()
    frame.icon = frame:CreateTexture(nil, "ARTWORK"); frame.icon:SetAllPoints(frame)
    frame.label = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); frame.label:SetPoint("TOP", frame, "BOTTOM", 0, -2)
    frame:SetScript("OnUpdate", function(item)
        local x, y = GetCursorPosition(); local scale = UIParent:GetEffectiveScale(); item:ClearAllPoints(); item:SetPoint("CENTER", UIParent, "BOTTOMLEFT", x / scale + 22, y / scale - 22)
    end)
    self.cursorFrame = frame
end

function Controller:BeginSystemAction(action, originLayer, originControl)
    if InCombatLockdown() then return false, "combat" end
    if GetCursorInfo() then ClearCursor() end
    local metadata = ThorPad.SystemActions:Get(action)
    self.internalCursor = { action = action, originLayer = originLayer, originControl = originControl }; self.cancelCursorNextFrame = false
    self:EnsureCursorFrame(); self.cursorFrame.icon:SetTexture(metadata and metadata.icon or [[Interface\Icons\INV_Misc_QuestionMark]]); self.cursorFrame.label:SetText(metadata and metadata.name or action); self.cursorFrame:Show()
    return true
end

function Controller:HasInternalCursor() return self.internalCursor ~= nil end
function Controller:ScheduleCursorCancel() if self.internalCursor then self.cancelCursorNextFrame = true end end
function Controller:ClearInternalCursor() self.internalCursor = nil; self.cancelCursorNextFrame = false; if self.cursorFrame then self.cursorFrame:Hide() end end
function Controller:CursorChanged() if GetCursorInfo() and self.internalCursor then self:ClearInternalCursor() end end

function Controller:TakeAssignment(layer, control)
    if InCombatLockdown() then return false, "combat" end
    local override = self:GetOverride(layer, control)
    if override then return self:BeginSystemAction(override.action, layer, control) end
    local actionID = self:GetActionID(layer, control)
    if not actionID or not HasAction(actionID) then return false, "empty" end
    self:ClearInternalCursor(); PickupAction(actionID); return true
end

function Controller:ClearNativeAction(actionID) if actionID and HasAction(actionID) then PickupAction(actionID); ClearCursor() end end

function Controller:SetOverride(layer, control, value)
    ThorPadDB.controller.assignments[layer][control] = value
    local button = self.buttons[layer] and self.buttons[layer][control]; if button then self:Apply(button) end
    self:MarkSyncDirty()
end

function Controller:DropOn(layer, control)
    if InCombatLockdown() then return false, "combat" end
    local actionID = self:GetActionID(layer, control); if not actionID then return false, "unsupported" end
    if self.internalCursor then
        local cursor = self.internalCursor
        self:ClearNativeAction(actionID)
        if cursor.originLayer and (cursor.originLayer ~= layer or cursor.originControl ~= control) then self:ClearNativeAction(self:GetActionID(cursor.originLayer, cursor.originControl)); self:SetOverride(cursor.originLayer, cursor.originControl, nil) end
        self:SetOverride(layer, control, { type = "system", action = cursor.action }); self:ClearInternalCursor(); return true
    end
    if not GetCursorInfo() then return false, "unsupported" end
    ThorPadDB.controller.assignments[layer][control] = nil; PlaceAction(actionID)
    local button = self.buttons[layer] and self.buttons[layer][control]; if button then self:Apply(button) end
    self:MarkSyncDirty(); return true
end

function Controller:ClearAssignment(layer, control)
    if InCombatLockdown() then return false, "combat" end
    if self:GetOverride(layer, control) then self:ClearNativeAction(self:GetActionID(layer, control)); self:SetOverride(layer, control, nil); return true end
    local actionID = self:GetActionID(layer, control)
    if not actionID or not HasAction(actionID) then return false, "empty" end
    PickupAction(actionID); ClearCursor(); return true
end

function Controller:GetState(descriptor)
    if descriptor and descriptor.type == "system" then return { icon = descriptor.icon, count = 0, usable = not descriptor.unknown, noResource = false, start = 0, duration = 0, enabled = 0, current = false } end
    local actionID = descriptor and descriptor.actionID; if not actionID then return nil end
    local usable, noResource = IsUsableAction(actionID); local start, duration, enabled = GetActionCooldown(actionID)
    return { icon = GetActionTexture(actionID), count = GetActionCount(actionID), usable = usable, noResource = noResource, start = start or 0, duration = duration or 0, enabled = enabled or 0, current = IsCurrentAction(actionID), inRange = IsActionInRange(actionID) }
end

function Controller:MarkSyncDirty() self.syncDirty = true end
function Controller:SyncSystemActions()
    if not self.syncDirty or not ThorPad.Native:IsSystemActionBridgeAvailable() then return false end
    if ThorPad.Native:ResetSystemActions() ~= true then return false end
    for _, layer in ipairs(C.CONTROLLER_LAYERS) do
        for _, control in ipairs(C.CONTROLLER_CONTROLS) do
            local assignment = self:GetOverride(layer, control)
            if assignment then
                if not InCombatLockdown() and not GetCursorInfo() then self:ClearNativeAction(self:GetActionID(layer, control)) end
                local supported = ThorPad.Native:SetSystemAction(layer, control, assignment.action)
                if supported == nil then return false end
                if not supported and not self.unknownLogged[assignment.action] then
                    self.unknownLogged[assignment.action] = true
                    DEFAULT_CHAT_FRAME:AddMessage("|cffff5050[ThorPad] Unknown System Action: " .. assignment.action .. "|r")
                end
            end
        end
    end
    self.syncDirty = false; return true
end

function Controller:Tick()
    if self.cancelCursorNextFrame then self:ClearInternalCursor() end
    if self.syncDirty then self:SyncSystemActions() end
end
