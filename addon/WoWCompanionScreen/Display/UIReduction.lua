local addonName, WCS = ...

WCS.UIReduction = { saved = {}, active = false, partyActive = false }
local Reduction = WCS.UIReduction

-- Blizzard's bottom action/menu chrome is reduced whenever the existing mode is active. Party frames
-- are managed separately and only replaced while the authenticated companion screen is connected.
Reduction.frames = {
    "MainMenuBar", "MainMenuBarArtFrame", "MainMenuBarOverlayFrame",
    "MultiBarBottomLeft", "MultiBarBottomRight", "BonusActionBarFrame",
    "PetActionBarFrame", "ShapeshiftBarFrame", "PossessBarFrame",
    "MultiCastActionBarFrame", "MultiCastFlyoutFrame", "VehicleMenuBar",
    "MainMenuExpBar", "MainMenuBarMaxLevelBar", "ReputationWatchBar", "ExhaustionTick",
    "CharacterMicroButton", "SpellbookMicroButton", "TalentMicroButton", "AchievementMicroButton",
    "QuestLogMicroButton", "SocialsMicroButton", "PVPMicroButton", "LFDMicroButton",
    "MainMenuMicroButton", "HelpMicroButton", "KeyRingButton", "CharacterBag3Slot",
    "CharacterBag2Slot", "CharacterBag1Slot", "CharacterBag0Slot", "MainMenuBarBackpackButton",
}
Reduction.partyFrames = { "PartyMemberFrame1", "PartyMemberFrame2", "PartyMemberFrame3", "PartyMemberFrame4" }

function Reduction:SetFrameHidden(name, hidden)
    local frame = _G[name]; if not frame then return end
    if hidden then
        if not self.saved[name] then
            self.saved[name] = { shown = frame:IsShown(), alpha = frame:GetAlpha(), mouse = frame.IsMouseEnabled and frame:IsMouseEnabled() }
        end
        frame:SetAlpha(0); if frame.EnableMouse then frame:EnableMouse(false) end; frame:Hide()
    elseif self.saved[name] then
        local state = self.saved[name]; frame:SetAlpha(state.alpha or 1); if frame.EnableMouse then frame:EnableMouse(state.mouse and true or false) end
        if state.shown then frame:Show() else frame:Hide() end; self.saved[name] = nil
    end
end

function Reduction:Apply(active, partyActive)
    self.active = active and true or false
    self.partyActive = partyActive and true or false
    for _, name in ipairs(self.frames) do self:SetFrameHidden(name, self.active) end
    for _, name in ipairs(self.partyFrames) do self:SetFrameHidden(name, self.partyActive) end
end

function Reduction:Reconcile()
    if self.active then for _, name in ipairs(self.frames) do self:SetFrameHidden(name, true) end end
    if self.partyActive then for _, name in ipairs(self.partyFrames) do self:SetFrameHidden(name, true) end end
end
