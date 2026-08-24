local addonName, ThorPad = ...
local eventFrame = CreateFrame("Frame")
local Core = { layer = 1, layerElapsed = 0, dockElapsed = 0 }

local function refreshActions()
    ThorPad.ActionOverlay:Refresh()
    if ThorPad.Settings.frame:IsShown() then ThorPad.Settings:Refresh() end
end
function Core:Initialize()
    ThorPadDB = ThorPadDB or {}
    ThorPadDB.minimap = ThorPadDB.minimap or {}
    if type(ThorPadDB.minimap.angle) ~= "number" then
        ThorPadDB.minimap.angle = ThorPad.Config.minimap.defaultAngle
    end
    ThorPad.ActionOverlay:Create()
    ThorPad.Settings:Create()
    ThorPad.UtilityDock:Create()
    ThorPad.XPBar:Create()
    ThorPad.MinimapButton:Create()
    self.layer = ThorPad.Native:GetCurrentLayer()
    ThorPad.ActionOverlay:SetLayer(self.layer)
    ThorPad.UtilityDock:Reconcile()
    ThorPad.XPBar:Refresh()
    if hooksecurefunc and UIParent_ManageFramePositions then
        hooksecurefunc("UIParent_ManageFramePositions", function()
            ThorPad.UtilityDock:Reconcile()
        end)
    end
end
eventFrame:RegisterEvent("PLAYER_LOGIN"); eventFrame:RegisterEvent("PLAYER_ENTERING_WORLD"); eventFrame:RegisterEvent("PLAYER_REGEN_ENABLED"); eventFrame:RegisterEvent("PLAYER_REGEN_DISABLED")
eventFrame:RegisterEvent("ACTIONBAR_SLOT_CHANGED"); eventFrame:RegisterEvent("ACTIONBAR_UPDATE_COOLDOWN"); eventFrame:RegisterEvent("ACTIONBAR_UPDATE_USABLE"); eventFrame:RegisterEvent("ACTIONBAR_UPDATE_STATE")
eventFrame:RegisterEvent("PLAYER_XP_UPDATE"); eventFrame:RegisterEvent("PLAYER_LEVEL_UP"); eventFrame:RegisterEvent("UPDATE_EXHAUSTION")
eventFrame:SetScript("OnEvent", function(_, event)
    if event == "PLAYER_LOGIN" then
        Core:Initialize()
        return
    end
    if event == "PLAYER_ENTERING_WORLD" or event == "PLAYER_REGEN_ENABLED" then
        ThorPad.UtilityDock:Reconcile()
    end
    if event == "PLAYER_XP_UPDATE" or event == "PLAYER_LEVEL_UP" or event == "UPDATE_EXHAUSTION" or event == "PLAYER_ENTERING_WORLD" then
        ThorPad.XPBar:Refresh()
    end
    if event == "ACTIONBAR_SLOT_CHANGED" or event == "ACTIONBAR_UPDATE_COOLDOWN" or event == "ACTIONBAR_UPDATE_USABLE" or event == "ACTIONBAR_UPDATE_STATE" then
        refreshActions()
    end
    if (event == "PLAYER_REGEN_ENABLED" or event == "PLAYER_REGEN_DISABLED") and ThorPad.Settings.frame:IsShown() then
        ThorPad.Settings:Refresh()
    end
end)
eventFrame:SetScript("OnUpdate", function(_, elapsed)
    if not ThorPad.ActionOverlay.frame then return end
    Core.layerElapsed = Core.layerElapsed + elapsed; Core.dockElapsed = Core.dockElapsed + elapsed
    if Core.layerElapsed >= ThorPad.Config.layerPollInterval then
        Core.layerElapsed = 0
        local layer = ThorPad.Native:GetCurrentLayer()
        if layer ~= Core.layer then
            Core.layer = layer
            ThorPad.ActionOverlay:SetLayer(layer)
        end
    end
    if Core.dockElapsed >= ThorPad.Config.dockReconcileInterval then
        Core.dockElapsed = 0
        ThorPad.UtilityDock:Reconcile()
    end
end)
SLASH_THORPAD1 = "/thorpad"
SlashCmdList.THORPAD = function()
    ThorPad.Settings:Toggle()
end
