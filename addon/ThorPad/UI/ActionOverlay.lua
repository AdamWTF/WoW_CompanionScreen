local addonName, ThorPad = ...
local C, Widgets = ThorPad.Constants, ThorPad.UI.Widgets

ThorPad.ActionOverlay = { layer = "default" }
local Overlay = ThorPad.ActionOverlay

function Overlay:Create()
    local frame = CreateFrame("Frame", "ThorPadActionOverlay", UIParent); frame:SetSize(520, 92); frame:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, 42); frame:SetFrameStrata("MEDIUM")
    self.frame, self.cells = frame, {}
    self.l2Indicator = frame:CreateTexture(nil, "OVERLAY"); self.l2Indicator:SetSize(28, 28); self.l2Indicator:SetPoint("BOTTOMRIGHT", frame, "TOP", -2, 2); self.l2Indicator:SetTexture(ThorPad.Glyphs:GetModifier("l2"))
    self.r2Indicator = frame:CreateTexture(nil, "OVERLAY"); self.r2Indicator:SetSize(28, 28); self.r2Indicator:SetPoint("BOTTOMLEFT", frame, "TOP", 2, 2); self.r2Indicator:SetTexture(ThorPad.Glyphs:GetModifier("r2"))
    for index, control in ipairs(C.CONTROLLER_CONTROLS) do
        local cell = Widgets:CreateActionCell(frame, nil, 52, "display"); cell:SetPoint("LEFT", frame, "LEFT", (index - 1) * 64 + 6, 0); cell.control = control; cell:EnableMouse(false); cell.label:Hide(); self.cells[index] = cell
    end
end

function Overlay:SetLayer(layer) self.layer = layer or "default"; self:Refresh() end
function Overlay:Refresh()
    if not self.frame then return end
    self.frame:SetScale(ThorPadDB.display.actionScale or 1)
    if self.layer == "l2" or self.layer == "l2r2" then self.l2Indicator:Show() else self.l2Indicator:Hide() end
    if self.layer == "r2" or self.layer == "l2r2" then self.r2Indicator:Show() else self.r2Indicator:Hide() end
    for _, cell in ipairs(self.cells) do cell.layer = self.layer; Widgets:RefreshControllerCell(cell) end
end
