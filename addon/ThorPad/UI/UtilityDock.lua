local addonName, ThorPad = ...

ThorPad.UtilityDock = {}
local Dock = ThorPad.UtilityDock
local microButtons = { "CharacterMicroButton", "SpellbookMicroButton", "TalentMicroButton", "AchievementMicroButton", "QuestLogMicroButton", "SocialsMicroButton", "PVPMicroButton", "LFDMicroButton", "MainMenuMicroButton", "HelpMicroButton" }
local bagButtons = { "KeyRingButton", "CharacterBag3Slot", "CharacterBag2Slot", "CharacterBag1Slot", "CharacterBag0Slot", "MainMenuBarBackpackButton" }

function Dock:Create()
    self.frame = CreateFrame("Frame", "ThorPadUtilityDock", UIParent); self.frame:SetSize(190, 180); self.frame:SetPoint("BOTTOMRIGHT", UIParent, "BOTTOMRIGHT", -12, 10); self.frame:SetFrameStrata("MEDIUM")
end
function Dock:HideHorizontalPresentation()
    for i = 1, 12 do local b = _G["ActionButton" .. i]; if b then b:SetAlpha(0) end end
    if MultiBarBottomLeft then MultiBarBottomLeft:SetAlpha(0) end
    if MultiBarBottomRight then MultiBarBottomRight:SetAlpha(0) end
    for i = 0, 3 do local art = _G["MainMenuBarTexture" .. i]; local xp = _G["MainMenuXPBarTexture" .. i]; if art then art:Hide() end; if xp then xp:Hide() end end
    if MainMenuBarLeftEndCap then MainMenuBarLeftEndCap:Hide() end; if MainMenuBarRightEndCap then MainMenuBarRightEndCap:Hide() end
    if MainMenuBarPageNumber then MainMenuBarPageNumber:Hide() end; if ActionBarUpButton then ActionBarUpButton:Hide() end; if ActionBarDownButton then ActionBarDownButton:Hide() end
    if MainMenuExpBar then MainMenuExpBar:SetAlpha(0) end; if ExhaustionTick then ExhaustionTick:SetAlpha(0) end
end
function Dock:Reconcile()
    self:HideHorizontalPresentation()
    if InCombatLockdown() then return end
    for index, name in ipairs(microButtons) do local b = _G[name]; if b then if b:GetParent() ~= self.frame then b:SetParent(self.frame) end; local col, row = math.mod(index - 1, 5), math.floor((index - 1) / 5); b:ClearAllPoints(); b:SetPoint("BOTTOMRIGHT", self.frame, "BOTTOMRIGHT", -2 - col * 32, 2 + row * 46) end end
    for index, name in ipairs(bagButtons) do local b = _G[name]; if b then if b:GetParent() ~= self.frame then b:SetParent(self.frame) end; b:ClearAllPoints(); b:SetPoint("BOTTOMRIGHT", self.frame, "BOTTOMRIGHT", -2 - (index - 1) * 36, 118) end end
end
