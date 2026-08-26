local addonName, ThorPad = ...

ThorPad.UIReduction = { saved = {}, active = false }
local Reduction = ThorPad.UIReduction

-- Explicitly limited to Blizzard's bottom action/menu chrome. Combat HUD,
-- unit frames, chat, minimap, buffs, cast bars, and objectives are untouched.
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

function Reduction:Apply(active)
    self.active = active and true or false
    for _, name in ipairs(self.frames) do self:SetFrameHidden(name, self.active) end
end

function Reduction:Reconcile()
    if self.active then for _, name in ipairs(self.frames) do self:SetFrameHidden(name, true) end end
end
