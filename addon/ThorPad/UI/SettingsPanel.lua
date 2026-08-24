local addonName, ThorPad = ...
local Config, Slots, Widgets = ThorPad.Config, ThorPad.ActionSlots, ThorPad.UI.Widgets

ThorPad.Settings = {}
local Settings = ThorPad.Settings

function Settings:Create()
    local panel = CreateFrame("Frame", "ThorPadConfig", UIParent, "UIPanelDialogTemplate")
    panel:SetSize(460, 360); panel:SetPoint("CENTER", UIParent, "CENTER", 0, 45); panel:SetFrameStrata("DIALOG"); panel:SetToplevel(true); panel:SetMovable(true); panel:EnableMouse(true); panel:RegisterForDrag("LeftButton")
    panel:SetScript("OnDragStart", function(self) self:StartMoving() end); panel:SetScript("OnDragStop", function(self) self:StopMovingOrSizing() end); panel:Hide()
    tinsert(UISpecialFrames, "ThorPadConfig")
    panel.title:SetText("ThorPad Controller Actions"); panel.title:SetJustifyH("CENTER")
    local note = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); note:SetPoint("TOP", panel, "TOP", 0, -39); note:SetText("Drag spells, macros or items here to place them on the controller bar.")
    self.status = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.status:SetPoint("BOTTOM", panel, "BOTTOM", 0, 16)
    self.frame, self.selectedTab, self.tabs, self.cells = panel, 1, {}, {}
    for i = 1, 5 do
        local tab = CreateFrame("Button", nil, panel, "UIPanelButtonTemplate"); tab:SetSize(68, 22); tab:SetPoint("TOPLEFT", panel, "TOPLEFT", 22 + (i - 1) * 80, -70); tab:SetText(Config.layerNames[i]); tab.index = i
        tab:SetScript("OnClick", function(button) self.selectedTab = button.index; self:Refresh() end); self.tabs[i] = tab
    end
    for i = 1, 8 do
        local p = Config.settingsPositions[i]; local cell = Widgets:CreateActionCell(panel, p[1], p[2], Config.settingsCellSize, true); cell.index = i; self.cells[i] = cell; self:BindCell(cell)
    end
    panel:SetScript("OnShow", function() self:Refresh() end)
    panel:RegisterEvent("PLAYER_REGEN_DISABLED"); panel:RegisterEvent("PLAYER_REGEN_ENABLED"); panel:SetScript("OnEvent", function() self:Refresh() end)
    panel.name = "ThorPad"; if InterfaceOptions_AddCategory then InterfaceOptions_AddCategory(panel) end
end

function Settings:CombatMessage() self.status:SetText("|cffff4040Leave combat to edit controller actions.|r") end
function Settings:BindCell(cell)
    cell:SetScript("OnEnter", function(button) if button.slot and button.slot > 0 then GameTooltip:SetOwner(button, "ANCHOR_RIGHT"); GameTooltip:SetAction(button.slot) end end)
    cell:SetScript("OnLeave", function() GameTooltip:Hide() end)
    cell:SetScript("OnReceiveDrag", function(button) self:PlaceCursorAction(button) end)
    cell:SetScript("OnDragStart", function(button) if not button.slot or button.slot == 0 then return end; if InCombatLockdown() then self:CombatMessage(); return end; PickupAction(button.slot) end)
    cell:SetScript("OnMouseUp", function(button, mouseButton)
        if not button.slot or button.slot == 0 then return end
        if mouseButton == "LeftButton" and GetCursorInfo() then self:PlaceCursorAction(button); self.status:SetText("Action placed in this controller cell.")
        elseif mouseButton == "RightButton" then if InCombatLockdown() then self:CombatMessage(); return end; PickupAction(button.slot); ClearCursor(); self:RefreshAll() end
    end)
end
function Settings:PlaceCursorAction(cell)
    if not cell.slot or cell.slot == 0 then return end
    if InCombatLockdown() then self:CombatMessage(); return end
    PlaceAction(cell.slot); self:RefreshAll()
end
function Settings:Refresh()
    for i = 1, 5 do local label = self.tabs[i]:GetFontString(); if i == self.selectedTab then self.tabs[i]:LockHighlight(); label:SetTextColor(1, 0.82, 0.15) else self.tabs[i]:UnlockHighlight(); label:SetTextColor(1, 1, 1) end end
    for i = 1, 8 do local cell = self.cells[i]; cell:SetAction(Slots:GetSlot(self.selectedTab, i)); cell:Refresh(i) end
    if InCombatLockdown() then self:CombatMessage() else self.status:SetText("Right-click an action to clear it.  R2 D-pad is intentionally unavailable.") end
end
function Settings:RefreshAll() self:Refresh(); ThorPad.ActionOverlay:Refresh() end
function Settings:Toggle() if self.frame:IsShown() then self.frame:Hide() else self.frame:Show() end end
