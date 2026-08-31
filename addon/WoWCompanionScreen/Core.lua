local addonName, WCS = ...
local eventFrame = CreateFrame("Frame")
local Core = { initialized = false, layer = "default", glyphSignature = "" }

local function printLine(message) DEFAULT_CHAT_FRAME:AddMessage("|cffffd040WCS:|r " .. message) end

function Core:Initialize()
    if self.initialized then return end; self.initialized = true
    WCS.Database:Initialize(); WCS.Controller:CreateButtons(); WCS.ActionOverlay:Create(); WCS.Settings:Create(); WCS.MinimapButton:Create(); WCS.Bridge:Initialize(); WCS.Display:Initialize(); WCS.UINavigation:Initialize()
    self.layer = WCS.Native:GetCurrentLayer(); self.glyphSignature = tostring(WCSGamepadConfiguredGlyphStyle) .. ":" .. tostring(WCSGamepadDetectedGlyphStyle); WCS.ActionOverlay:SetLayer(self.layer); WCS.Display:Apply()
end

function Core:RefreshActions()
    WCS.ActionOverlay:Refresh(); if WCS.Settings.frame and WCS.Settings.frame:IsShown() then WCS.Settings:RefreshActions() end
end

local events = { "PLAYER_LOGIN", "PLAYER_ENTERING_WORLD", "PLAYER_REGEN_ENABLED", "PLAYER_REGEN_DISABLED", "ACTIONBAR_SLOT_CHANGED", "ACTIONBAR_UPDATE_COOLDOWN", "ACTIONBAR_UPDATE_USABLE", "ACTIONBAR_UPDATE_STATE", "ACTIONBAR_PAGE_CHANGED", "UPDATE_BONUS_ACTIONBAR", "UPDATE_SHAPESHIFT_FORM", "SPELLS_CHANGED", "BAG_UPDATE", "UPDATE_BINDINGS", "CURSOR_UPDATE", "PLAYER_TARGET_CHANGED", "PLAYER_LEVEL_UP", "PLAYER_XP_UPDATE", "UPDATE_EXHAUSTION", "PLAYER_MONEY" }
for _, event in ipairs(events) do eventFrame:RegisterEvent(event) end
eventFrame:SetScript("OnEvent", function(_, event)
    if event == "PLAYER_LOGIN" then Core:Initialize(); return end
    if not Core.initialized then return end
    if event == "CURSOR_UPDATE" then WCS.Controller:CursorChanged(); return end
    WCS.Bridge:OnEvent(event)
    WCS.UINavigation:OnEvent(event)
    if event == "ACTIONBAR_PAGE_CHANGED" or event == "UPDATE_BONUS_ACTIONBAR" or event == "UPDATE_SHAPESHIFT_FORM" then WCS.Controller:ResolveMainActions() end
    if event == "PLAYER_REGEN_ENABLED" then WCS.Controller:ApplyAll(); WCS.Controller:MarkSyncDirty(); WCS.Display:Apply() end
    if event == "PLAYER_ENTERING_WORLD" then WCS.Controller:MarkSyncDirty() end
    if event == "PLAYER_ENTERING_WORLD" or event == "PLAYER_REGEN_ENABLED" then WCS.Display:Reconcile() end
    if event == "PLAYER_REGEN_DISABLED" and WCS.Settings.frame:IsShown() then WCS.Settings:SetStatus("Configuration changes are locked during combat.", true) end
    Core:RefreshActions()
end)

eventFrame:SetScript("OnUpdate", function(_, elapsed)
    if not Core.initialized then return end
    WCS.Controller:Tick(elapsed)
    WCS.Bridge:Tick(elapsed)
    WCS.UINavigation:Tick(elapsed)
    Core.layerElapsed = (Core.layerElapsed or 0) + elapsed; Core.reconcileElapsed = (Core.reconcileElapsed or 0) + elapsed
    if Core.layerElapsed >= .12 then
        Core.layerElapsed = 0; local layer = WCS.Native:GetCurrentLayer(); local glyphSignature = tostring(WCSGamepadConfiguredGlyphStyle) .. ":" .. tostring(WCSGamepadDetectedGlyphStyle)
        if layer ~= Core.layer then Core.layer = layer; WCS.ActionOverlay:SetLayer(layer)
        elseif glyphSignature ~= Core.glyphSignature then WCS.ActionOverlay:Refresh() end
        Core.glyphSignature = glyphSignature
    end
    if Core.reconcileElapsed >= 2 then Core.reconcileElapsed = 0; WCS.Display:Reconcile() end
end)

SLASH_WCS1, SLASH_WCS2 = "/wcs", "/wowcompanionscreen"
SlashCmdList.WCS = function(message)
    local command = string.lower((message or ""):match("^%s*(.-)%s*$"))
    if command == "mode" then
        printLine("Controller: " .. (WCSDB.controller.enabled and "Enabled" or "Disabled")); printLine("Second Screen: " .. (WCSDB.secondScreen.enabled and "Enabled" or "Disabled")); printLine("Reduced UI: " .. ((WCSDB.controller.enabled and WCSDB.secondScreen.enabled and WCSDB.secondScreen.reduceUI) and "Enabled" or "Disabled")); printLine("Mode: " .. WCS.Display:GetMode())
    elseif command == "debug" then WCSDB.debug = not WCSDB.debug; printLine("Debug logging " .. (WCSDB.debug and "enabled" or "disabled") .. ".")
    elseif command == "reset" then WCS.Controller:ClearInternalCursor(); WCS.Database:Reset(); WCS.Controller:ApplyAll(); WCS.Controller:MarkSyncDirty(); WCS.Display:Apply(); WCS.Settings:RefreshAll(); printLine("Settings and controller assignments reset.")
    else WCS.Settings:Toggle() end
end
