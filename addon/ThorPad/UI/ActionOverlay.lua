local addonName, ThorPad = ...
local Config, Slots, Widgets = ThorPad.Config, ThorPad.ActionSlots, ThorPad.UI.Widgets

ThorPad.ActionOverlay = {}
local Overlay = ThorPad.ActionOverlay

function Overlay:Create()
    self.frame = CreateFrame("Frame", "ThorPadActionOverlay", UIParent)
    self.frame:SetSize(470, 170); self.frame:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, 34); self.frame:SetFrameStrata("MEDIUM")
    self.labels, self.cells = {}, {}
    for i, name in ipairs({ "L1", "L2", "R1", "R2" }) do
        local badge = CreateFrame("Frame", nil, self.frame); badge:SetSize(30, 18); badge:SetPoint("TOP", self.frame, "TOP", -54 + (i - 1) * 36, -16); Widgets:SetBackdrop(badge)
        local text = badge:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall"); text:SetPoint("CENTER", badge, "CENTER"); text:SetText(name)
        self.labels[i] = { frame = badge, text = text }
    end
    for i = 1, 8 do local p = Config.overlayPositions[i]; self.cells[i] = Widgets:CreateActionCell(self.frame, p[1], p[2], Config.overlayCellSize, false) end
end

function Overlay:SetLayer(layer)
    self.layer = layer
    for i = 1, 4 do
        local active = layer == i + 1
        self.labels[i].text:SetTextColor(active and 1 or 0.55, active and 0.82 or 0.55, active and 0.15 or 0.55)
        self.labels[i].frame:SetBackdropBorderColor(active and 1 or 0.35, active and 0.72 or 0.35, active and 0.15 or 0.35, active and 1 or 0.8)
    end
    self:Refresh()
end

function Overlay:Refresh()
    if not self.cells then return end
    for i = 1, 8 do local cell = self.cells[i]; cell:SetAction(Slots:GetSlot(self.layer or 1, i)); cell:Refresh(i) end
end
