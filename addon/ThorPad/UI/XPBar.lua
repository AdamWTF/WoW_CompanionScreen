local addonName, ThorPad = ...
local Widgets = ThorPad.UI.Widgets
ThorPad.XPBar = {}
function ThorPad.XPBar:Create()
    local bar = CreateFrame("StatusBar", "ThorPadXPBar", UIParent); bar:SetSize(430, 14); bar:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, 8); bar:SetStatusBarTexture([[Interface\TargetingFrame\UI-StatusBar]]); bar:SetStatusBarColor(0.16, 0.38, 0.82, 0.96); Widgets:SetBackdrop(bar)
    self.frame = bar; self.xp = bar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.xp:SetPoint("CENTER", bar, "CENTER")
    self.level = bar:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall"); self.level:SetPoint("LEFT", bar, "LEFT", 7, 0)
    self.rested = bar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.rested:SetPoint("RIGHT", bar, "RIGHT", -7, 0)
end
function ThorPad.XPBar:Refresh()
    local current, maximum = UnitXP("player"), UnitXPMax("player")
    if not maximum or maximum == 0 then self.frame:Hide(); return end
    self.frame:SetMinMaxValues(0, maximum); self.frame:SetValue(current); self.level:SetText(UnitLevel("player")); self.xp:SetText(string.format("%d / %d XP", current, maximum))
    self.rested:SetText((GetXPExhaustion and (GetXPExhaustion() or 0) > 0) and "ZZ" or ""); self.frame:Show()
end
