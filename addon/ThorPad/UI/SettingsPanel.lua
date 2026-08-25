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
    local note = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); note:SetPoint("TOP", panel, "TOP", 0, -39); note:SetText("Drag spells, macros or items here to place them on the controller bar."); self.note = note
    self.status = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.status:SetPoint("BOTTOM", panel, "BOTTOM", 0, 16)
    self.frame, self.selectedTab, self.tabs, self.cells = panel, 1, {}, {}
    for i = 1, 6 do
        local tab = CreateFrame("Button", nil, panel, "UIPanelButtonTemplate"); tab:SetSize(62, 22); tab:SetPoint("TOPLEFT", panel, "TOPLEFT", 16 + (i - 1) * 70, -70); tab:SetText(Config.layerNames[i]); tab.index = i
        tab:SetScript("OnClick", function(button) self.selectedTab = button.index; self:Refresh() end); self.tabs[i] = tab
    end
    for i = 1, 8 do
        local p = Config.settingsPositions[i]; local cell = Widgets:CreateActionCell(panel, p[1], p[2], Config.settingsCellSize, true); cell.index = i; self.cells[i] = cell; self:BindCell(cell)
    end
    self:CreateBridgePanel(panel)
    panel:SetScript("OnShow", function() self:Refresh() end)
    panel:RegisterEvent("PLAYER_REGEN_DISABLED"); panel:RegisterEvent("PLAYER_REGEN_ENABLED"); panel:SetScript("OnEvent", function() self:Refresh() end)
    panel.name = "ThorPad"; if InterfaceOptions_AddCategory then InterfaceOptions_AddCategory(panel) end
end

function Settings:CreateBridgePanel(parent)
    local bridge = CreateFrame("Frame", nil, parent); bridge:SetPoint("TOPLEFT", parent, "TOPLEFT", 28, -110); bridge:SetPoint("BOTTOMRIGHT", parent, "BOTTOMRIGHT", -28, 48); bridge:Hide()
    local title = bridge:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge"); title:SetPoint("TOPLEFT", bridge, "TOPLEFT", 0, 0); title:SetText("Thor Bridge")
    self.bridgeStatus = bridge:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); self.bridgeStatus:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -18); self.bridgeStatus:SetJustifyH("LEFT")
    self.bridgeEndpoint = bridge:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.bridgeEndpoint:SetPoint("TOPLEFT", self.bridgeStatus, "BOTTOMLEFT", 0, -10); self.bridgeEndpoint:SetJustifyH("LEFT")
    self.bridgePairing = bridge:CreateFontString(nil, "OVERLAY", "GameFontNormal"); self.bridgePairing:SetPoint("TOPLEFT", self.bridgeEndpoint, "BOTTOMLEFT", 0, -18); self.bridgePairing:SetJustifyH("LEFT")
    local forget = CreateFrame("Button", nil, bridge, "UIPanelButtonTemplate"); forget:SetSize(150, 24); forget:SetPoint("BOTTOMLEFT", bridge, "BOTTOMLEFT", 0, 0); forget:SetText("Forget paired device")
    forget:SetScript("OnClick", function() StaticPopup_Show("THORPAD_FORGET_BRIDGE_DEVICE") end)
    self.bridgePanel, self.bridgeForget = bridge, forget
    StaticPopupDialogs["THORPAD_FORGET_BRIDGE_DEVICE"] = {
        text = "Forget the paired Thor device and disconnect it? A new pairing code will be generated.", button1 = YES, button2 = NO,
        OnAccept = function() ThorPad.Native:ForgetBridgeDevice(); Settings:RefreshBridge() end,
        timeout = 0, whileDead = true, hideOnEscape = true, preferredIndex = 3,
    }
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
    for i = 1, 6 do local label = self.tabs[i]:GetFontString(); if i == self.selectedTab then self.tabs[i]:LockHighlight(); label:SetTextColor(1, 0.82, 0.15) else self.tabs[i]:UnlockHighlight(); label:SetTextColor(1, 1, 1) end end
    local bridgeSelected = self.selectedTab == 6
    if bridgeSelected then self.note:Hide(); self.bridgePanel:Show() else self.note:Show(); self.bridgePanel:Hide() end
    for i = 1, 8 do local cell = self.cells[i]; if bridgeSelected then cell:Hide() else cell:Show(); cell:SetAction(Slots:GetSlot(self.selectedTab, i)); cell:Refresh(i) end end
    if bridgeSelected then self:RefreshBridge(); self.status:SetText(""); return end
    if InCombatLockdown() then self:CombatMessage() else self.status:SetText("Right-click an action to clear it.  R2 D-pad is intentionally unavailable.") end
end

function Settings:RefreshBridge()
    if not self.bridgePanel or self.selectedTab ~= 6 or not self.frame:IsShown() then return end
    local state = ThorPad.Bridge:GetStatus()
    if not state.available then
        self.bridgeStatus:SetText("|cffff4040Thor Bridge DLL is not available.|r"); self.bridgeEndpoint:SetText(""); self.bridgePairing:SetText(""); self.bridgeForget:Hide(); return
    end
    local network = not state.enabled and "disabled" or (state.listening and "listening" or "failed to listen")
    local client = state.connected and "client connected" or "no active client"
    self.bridgeStatus:SetText(string.format("Network: %s   -   %s", network, client))
    self.bridgeEndpoint:SetText(string.format("Endpoint: ws://%s:%d/thor", state.bindAddress or "0.0.0.0", state.port or 18423))
    if state.paired then self.bridgePairing:SetText("Paired device: " .. ((state.device ~= "" and state.device) or "Unknown device")); self.bridgeForget:Show()
    else self.bridgePairing:SetText("Pairing code: |cffffd040" .. ((state.pairingCode ~= "" and state.pairingCode) or "Unavailable") .. "|r"); self.bridgeForget:Hide() end
end
function Settings:RefreshAll() self:Refresh(); ThorPad.ActionOverlay:Refresh() end
function Settings:Toggle() if self.frame:IsShown() then self.frame:Hide() else self.frame:Show() end end
