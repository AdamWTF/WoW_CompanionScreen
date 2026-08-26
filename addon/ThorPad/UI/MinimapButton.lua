local addonName, ThorPad = ...

ThorPad.MinimapButton = {}
local MinimapButton = ThorPad.MinimapButton

function MinimapButton:Create()
    local button = CreateFrame("Button", "ThorPadMinimapButton", Minimap); button:SetSize(32, 32); button:SetFrameStrata("MEDIUM"); button:SetFrameLevel(8); button:RegisterForClicks("LeftButtonUp"); button:RegisterForDrag("LeftButton")
    local background = button:CreateTexture(nil, "BACKGROUND"); background:SetTexture([[Interface\Minimap\UI-Minimap-Background]]); background:SetSize(24, 24); background:SetPoint("CENTER")
    local icon = button:CreateTexture(nil, "ARTWORK"); icon:SetTexture(ThorPad.Constants.MEDIA .. "thorpad_icon"); icon:SetSize(24, 24); icon:SetPoint("CENTER"); icon:SetTexCoord(0, 1, 0, 1)
    local border = button:CreateTexture(nil, "OVERLAY"); border:SetTexture([[Interface\Minimap\MiniMap-TrackingBorder]]); border:SetSize(54, 54); border:SetPoint("TOPLEFT")
    local highlight = button:CreateTexture(nil, "HIGHLIGHT"); highlight:SetTexture([[Interface\Minimap\UI-Minimap-ZoomButton-Highlight]]); highlight:SetBlendMode("ADD"); highlight:SetSize(32, 32); highlight:SetPoint("CENTER")
    button:SetScript("OnClick", function() ThorPad.Settings:Toggle() end)
    button:SetScript("OnEnter", function(self) GameTooltip:SetOwner(self, "ANCHOR_LEFT"); GameTooltip:SetText("ThorPad"); GameTooltip:AddLine("Left-click to open settings.", 1, 1, 1); GameTooltip:Show() end); button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    button:SetScript("OnDragStart", function(self) self.dragging = true end); button:SetScript("OnDragStop", function(self) self.dragging = nil; MinimapButton:UpdateAngleFromCursor() end)
    button:SetScript("OnUpdate", function(self) if self.dragging then MinimapButton:UpdateAngleFromCursor() end end); self.button = button; self:Position()
    if ThorPadDB.minimap.hidden then button:Hide() end
end
function MinimapButton:Position() local angle = ThorPadDB.minimap.angle * math.pi / 180; self.button:ClearAllPoints(); self.button:SetPoint("CENTER", Minimap, "CENTER", math.cos(angle) * 80, math.sin(angle) * 80) end
function MinimapButton:UpdateAngleFromCursor()
    local scale = Minimap:GetEffectiveScale(); local x, y = GetCursorPosition(); x, y = x / scale - Minimap:GetLeft(), y / scale - Minimap:GetBottom()
    ThorPadDB.minimap.angle = math.deg(math.atan2(y - Minimap:GetHeight() / 2, x - Minimap:GetWidth() / 2)); self:Position()
end
