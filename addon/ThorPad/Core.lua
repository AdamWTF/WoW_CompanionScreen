local addonName, ThorPad = ...
local eventFrame = CreateFrame("Frame")
local Core = { initialized = false, layer = "default", glyphSignature = "" }

local function printLine(message) DEFAULT_CHAT_FRAME:AddMessage("|cffffd040ThorPad:|r " .. message) end

function Core:Initialize()
    if self.initialized then return end; self.initialized = true
    ThorPad.Database:Initialize(); ThorPad.Controller:CreateButtons(); ThorPad.ActionOverlay:Create(); ThorPad.Settings:Create(); ThorPad.MinimapButton:Create(); ThorPad.Bridge:Initialize(); ThorPad.Display:Initialize()
    self.layer = ThorPad.Native:GetCurrentLayer(); self.glyphSignature = tostring(WXLGamepadConfiguredGlyphStyle) .. ":" .. tostring(WXLGamepadDetectedGlyphStyle); ThorPad.ActionOverlay:SetLayer(self.layer); ThorPad.Display:Apply()
end

function Core:RefreshActions()
    ThorPad.ActionOverlay:Refresh(); if ThorPad.Settings.frame and ThorPad.Settings.frame:IsShown() then ThorPad.Settings:RefreshActions() end
end

local events = { "PLAYER_LOGIN", "PLAYER_ENTERING_WORLD", "PLAYER_REGEN_ENABLED", "PLAYER_REGEN_DISABLED", "ACTIONBAR_SLOT_CHANGED", "ACTIONBAR_UPDATE_COOLDOWN", "ACTIONBAR_UPDATE_USABLE", "ACTIONBAR_UPDATE_STATE", "SPELLS_CHANGED", "BAG_UPDATE", "UPDATE_BINDINGS", "CURSOR_UPDATE", "PLAYER_TARGET_CHANGED", "PLAYER_LEVEL_UP", "PLAYER_XP_UPDATE", "UPDATE_EXHAUSTION", "PLAYER_MONEY" }
for _, event in ipairs(events) do eventFrame:RegisterEvent(event) end
eventFrame:SetScript("OnEvent", function(_, event)
    if event == "PLAYER_LOGIN" then Core:Initialize(); return end
    if not Core.initialized then return end
    if event == "CURSOR_UPDATE" then ThorPad.Controller:CursorChanged(); return end
    ThorPad.Bridge:OnEvent(event)
    if event == "PLAYER_REGEN_ENABLED" then ThorPad.Controller:ApplyAll(); ThorPad.Display:Apply() end
    if event == "PLAYER_ENTERING_WORLD" or event == "PLAYER_REGEN_ENABLED" then ThorPad.Display:Reconcile() end
    if event == "PLAYER_REGEN_DISABLED" and ThorPad.Settings.frame:IsShown() then ThorPad.Settings:SetStatus("Configuration changes are locked during combat.", true) end
    Core:RefreshActions()
end)

eventFrame:SetScript("OnUpdate", function(_, elapsed)
    if not Core.initialized then return end
    ThorPad.Bridge:Tick(elapsed)
    Core.layerElapsed = (Core.layerElapsed or 0) + elapsed; Core.reconcileElapsed = (Core.reconcileElapsed or 0) + elapsed
    if Core.layerElapsed >= .12 then
        Core.layerElapsed = 0; local layer = ThorPad.Native:GetCurrentLayer(); local glyphSignature = tostring(WXLGamepadConfiguredGlyphStyle) .. ":" .. tostring(WXLGamepadDetectedGlyphStyle)
        if layer ~= Core.layer then Core.layer = layer; ThorPad.ActionOverlay:SetLayer(layer)
        elseif glyphSignature ~= Core.glyphSignature then ThorPad.ActionOverlay:Refresh() end
        Core.glyphSignature = glyphSignature
    end
    if Core.reconcileElapsed >= 2 then Core.reconcileElapsed = 0; ThorPad.Display:Reconcile() end
end)

SLASH_THORPAD1, SLASH_THORPAD2 = "/thorpad", "/tp"
SlashCmdList.THORPAD = function(message)
    local command = string.lower((message or ""):match("^%s*(.-)%s*$"))
    if command == "mode" then
        printLine("Controller: " .. (ThorPadDB.controller.enabled and "Enabled" or "Disabled")); printLine("Second Screen: " .. (ThorPadDB.secondScreen.enabled and "Enabled" or "Disabled")); printLine("Reduced UI: " .. ((ThorPadDB.controller.enabled and ThorPadDB.secondScreen.enabled and ThorPadDB.secondScreen.reduceUI) and "Enabled" or "Disabled")); printLine("Mode: " .. ThorPad.Display:GetMode())
    elseif command == "debug" then ThorPadDB.debug = not ThorPadDB.debug; printLine("Debug logging " .. (ThorPadDB.debug and "enabled" or "disabled") .. ".")
    elseif command == "reset" then ThorPad.Database:Reset(); ThorPad.Controller:ApplyAll(); ThorPad.Display:Apply(); ThorPad.Settings:RefreshAll(); printLine("Settings and controller assignments reset.")
    else ThorPad.Settings:Toggle() end
end
