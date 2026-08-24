local addonName, ThorPad = ...
local Config = ThorPad.Config
ThorPad.MinimapButton = {}
function ThorPad.MinimapButton:Create()
    local button = CreateFrame("Button", "ThorPadMinimapButton", Minimap)
    button:SetSize(32, 32)
    button:SetFrameStrata("MEDIUM")
    button:SetFrameLevel(8)
    button:RegisterForClicks("LeftButtonUp")
    button:RegisterForDrag("LeftButton")

    local background = button:CreateTexture(nil, "BACKGROUND")
    background:SetTexture([[Interface\Minimap\UI-Minimap-Background]])
    background:SetSize(24, 24)
    background:SetPoint("CENTER", button, "CENTER", 0, 0)

    local icon = button:CreateTexture(nil, "ARTWORK")
    icon:SetTexture(Config.minimapTexture)
    icon:SetSize(23, 23)
    icon:SetPoint("CENTER", button, "CENTER", 0, 0)
    icon:SetTexCoord(0.12, 0.88, 0.12, 0.88)

    -- This texture contains its shadow outside the visible ring. On 3.3.5a it
    -- must retain its native 54px geometry rather than being squeezed to 32px.
    local border = button:CreateTexture(nil, "OVERLAY")
    border:SetTexture([[Interface\Minimap\MiniMap-TrackingBorder]])
    border:SetSize(54, 54)
    border:SetPoint("TOPLEFT", button, "TOPLEFT", 0, 0)

    local highlight = button:CreateTexture(nil, "HIGHLIGHT")
    highlight:SetTexture([[Interface\Minimap\UI-Minimap-ZoomButton-Highlight]])
    highlight:SetBlendMode("ADD")
    highlight:SetSize(32, 32)
    highlight:SetPoint("CENTER", button, "CENTER", 0, 0)
    button:SetScript("OnClick", function(_, mouseButton) if mouseButton == "LeftButton" then ThorPad.Settings:Toggle() end end)
    button:SetScript("OnEnter", function(self) GameTooltip:SetOwner(self, "ANCHOR_LEFT"); GameTooltip:SetText("ThorPad"); GameTooltip:AddLine("Click to configure controller actions", 1, 1, 1); GameTooltip:Show() end)
    button:SetScript("OnLeave", function() GameTooltip:Hide() end)
    button:SetScript("OnDragStart", function(self) self.dragging = true end); button:SetScript("OnDragStop", function(self) self.dragging = nil; self:UpdateAngleFromCursor() end)
    button:SetScript("OnUpdate", function(self) if self.dragging then self:UpdateAngleFromCursor() end end)
    self.button = button; self:Position()
end
function ThorPad.MinimapButton:GetAngle() return ThorPadDB.minimap.angle end
function ThorPad.MinimapButton:Position()
    local angle = self:GetAngle() * math.pi / 180; local radius = Config.minimap.radius
    self.button:ClearAllPoints(); self.button:SetPoint("CENTER", Minimap, "CENTER", math.cos(angle) * radius, math.sin(angle) * radius)
end
function ThorPad.MinimapButton:UpdateAngleFromCursor()
    local scale = Minimap:GetEffectiveScale(); local x, y = GetCursorPosition(); x, y = x / scale - Minimap:GetLeft(), y / scale - Minimap:GetBottom()
    ThorPadDB.minimap.angle = math.deg(math.atan2(y - Minimap:GetHeight() / 2, x - Minimap:GetWidth() / 2)); self:Position()
end
