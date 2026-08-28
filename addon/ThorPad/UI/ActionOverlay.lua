local addonName, ThorPad = ...
local C, Widgets = ThorPad.Constants, ThorPad.UI.Widgets

ThorPad.ActionOverlay = { layer = "default" }
local Overlay = ThorPad.ActionOverlay

local CELL_SIZE, STEP, PADDING, GROUP_GAP, GLYPH_SPACE = 52, 58, 6, 16, 18
local GROUP_SIZE = CELL_SIZE + STEP * 2
local RIGHT_GROUP_X = PADDING + GROUP_SIZE + GROUP_GAP
local POSITIONS = {
    dpad_up = { PADDING + STEP, -PADDING },
    dpad_left = { PADDING, -PADDING - STEP },
    dpad_right = { PADDING + STEP * 2, -PADDING - STEP },
    dpad_down = { PADDING + STEP, -PADDING - STEP * 2 },
    north = { RIGHT_GROUP_X + STEP, -PADDING },
    west = { RIGHT_GROUP_X, -PADDING - STEP },
    east = { RIGHT_GROUP_X + STEP * 2, -PADDING - STEP },
    south = { RIGHT_GROUP_X + STEP, -PADDING - STEP * 2 },
}

function Overlay:Create()
    local frameWidth = PADDING * 2 + GROUP_SIZE * 2 + GROUP_GAP
    local frameHeight = PADDING + GROUP_SIZE + GLYPH_SPACE
    local frame = CreateFrame("Frame", "ThorPadActionOverlay", UIParent); frame:SetSize(frameWidth, frameHeight); frame:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, 42); frame:SetFrameStrata("MEDIUM")
    self.frame, self.cells = frame, {}
    self.l2Indicator = frame:CreateTexture(nil, "OVERLAY"); self.l2Indicator:SetSize(28, 28); self.l2Indicator:SetPoint("BOTTOMRIGHT", frame, "TOP", -2, 2); self.l2Indicator:SetTexture(ThorPad.Glyphs:GetModifier("l2"))
    self.r2Indicator = frame:CreateTexture(nil, "OVERLAY"); self.r2Indicator:SetSize(28, 28); self.r2Indicator:SetPoint("BOTTOMLEFT", frame, "TOP", 2, 2); self.r2Indicator:SetTexture(ThorPad.Glyphs:GetModifier("r2"))
    for index, control in ipairs(C.CONTROLLER_CONTROLS) do
        local position = POSITIONS[control]
        local cell = Widgets:CreateActionCell(frame, nil, CELL_SIZE, "display"); cell:SetPoint("TOPLEFT", frame, "TOPLEFT", position[1], position[2]); cell.control = control; cell:EnableMouse(false); cell.label:Hide(); self.cells[index] = cell
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
