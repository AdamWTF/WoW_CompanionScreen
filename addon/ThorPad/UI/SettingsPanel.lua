local addonName, ThorPad = ...
local C, Widgets = ThorPad.Constants, ThorPad.UI.Widgets

ThorPad.Settings = { selectedPage = 1, selectedLayer = "default" }
local Settings = ThorPad.Settings

local function showTooltip(cell, descriptor)
    GameTooltip:SetOwner(cell, "ANCHOR_RIGHT")
    if cell.actionID then GameTooltip:SetAction(cell.actionID)
    elseif descriptor and descriptor.type == "spell" and descriptor.bookIndex and descriptor.bookType and GameTooltip.SetSpell then GameTooltip:SetSpell(descriptor.bookIndex, descriptor.bookType)
    elseif descriptor and descriptor.type == "item" and type(descriptor.item) == "string" and GameTooltip.SetHyperlink then GameTooltip:SetHyperlink(descriptor.item)
    elseif descriptor and descriptor.actionID then GameTooltip:SetAction(descriptor.actionID)
    elseif descriptor then GameTooltip:SetText(descriptor.name or C.CONTROLLER_CONTROL_LABELS[cell.control] or "ThorPad action"); GameTooltip:AddLine(descriptor.type, .7, .7, .7) end
    GameTooltip:Show()
end

function Settings:SetStatus(message, errorMessage) self.status:SetText((errorMessage and "|cffff5050" or "|cffb8b8b8") .. (message or "") .. "|r") end

function Settings:CreateControllerPage(parent)
    local page = CreateFrame("Frame", nil, parent); page:SetAllPoints(parent); page:SetFrameLevel(parent:GetFrameLevel() + 5); self.pages[1] = page
    local title = page:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge"); title:SetPoint("TOPLEFT", page, "TOPLEFT", 18, -15); title:SetText("Controller Mapping")
    local help = page:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); help:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -8); help:SetText("Drag WoW actions or ThorPad System Actions onto any controller position.")
    self.layerTabs, self.controllerCells = {}, {}
    for index, layer in ipairs(C.CONTROLLER_LAYERS) do
        local tab = CreateFrame("Button", nil, page, "UIPanelButtonTemplate"); tab:SetSize(104, 24); tab:SetPoint("TOPLEFT", page, "TOPLEFT", 18 + (index - 1) * 112, -66); tab:SetText(C.CONTROLLER_LAYER_LABELS[layer]); tab.layer = layer
        tab:SetScript("OnClick", function(button) self.selectedLayer = button.layer; self:RefreshControllerPage() end); self.layerTabs[index] = tab
    end
    for index, control in ipairs(C.CONTROLLER_CONTROLS) do
        local cell = Widgets:CreateActionCell(page, nil, 68, "controller"); local row = index <= 4 and 0 or 1; local column = (index - 1) % 4
        cell:SetPoint("TOPLEFT", page, "TOPLEFT", 48 + column * 112, -132 - row * 126); cell.control = control; cell.label:SetText(C.CONTROLLER_CONTROL_LABELS[control]); self.controllerCells[index] = cell
        cell:SetScript("OnEnter", function(button) showTooltip(button, ThorPad.Controller:GetAssignment(self.selectedLayer, button.control)) end); cell:SetScript("OnLeave", function() GameTooltip:Hide() end)
        cell:SetScript("OnDragStart", function(button)
            if InCombatLockdown() then self:SetStatus("Leave combat to edit protected controller actions.", true); return end
            if ThorPad.Controller:TakeAssignment(self.selectedLayer, button.control) then self:RefreshActions() end
        end)
        cell:SetScript("OnDragStop", function() ThorPad.Controller:ScheduleCursorCancel() end)
        local function drop(button)
            local ok, reason = ThorPad.Controller:DropOn(self.selectedLayer, button.control)
            if not ok then self:SetStatus(reason == "combat" and "Leave combat to edit action slots." or "That cursor payload is not supported.", true) else self:SetStatus("Controller action slot updated.") end
            self:RefreshActions()
        end
        cell:SetScript("OnReceiveDrag", drop)
        cell:SetScript("OnMouseUp", function(button, mouseButton)
            if mouseButton == "RightButton" then
                if InCombatLockdown() then self:SetStatus("Leave combat to edit protected controller actions.", true)
                else ThorPad.Controller:ClearAssignment(self.selectedLayer, button.control); self:SetStatus("Controller action slot cleared.") end
                self:RefreshActions()
            elseif GetCursorInfo() or ThorPad.Controller:HasInternalCursor() then drop(button) end
        end)
    end
    local systemSection = Widgets:CreateSection(page, "System Actions", 18, -356, 484, 82)
    self.systemActionCells = {}
    local function createSystemActionCell(action, index)
        local cell = Widgets:CreateActionCell(systemSection, nil, 48, "system"); cell:SetPoint("TOPLEFT", systemSection, "TOPLEFT", 18 + (index - 1) * 62, -26); cell.label:Hide(); cell.glyphFrame:Hide(); cell.actionLabel:SetText(action.name)
        Widgets:SetVisualState(cell, action.icon, 0, true, false, 0, 0, 0, false, nil); cell.systemAction = action.id; self.systemActionCells[action.id] = cell
        cell:SetScript("OnEnter", function(button) showTooltip(button, { type = "system", name = action.name }) end); cell:SetScript("OnLeave", function() GameTooltip:Hide() end)
        cell:SetScript("OnDragStart", function(button)
            local ok, reason = ThorPad.Controller:BeginSystemAction(button.systemAction)
            if not ok and reason == "combat" then self:SetStatus("Leave combat to edit protected controller actions.", true) end
        end)
        cell:SetScript("OnDragStop", function() ThorPad.Controller:ScheduleCursorCancel() end)
    end
    local systemIndex = 0; for _, action in ipairs(ThorPad.SystemActions:All()) do systemIndex = systemIndex + 1; createSystemActionCell(action, systemIndex) end
end

function Settings:CreateSecondScreenPage(parent)
    local page = CreateFrame("Frame", nil, parent); page:SetAllPoints(parent); page:SetFrameLevel(parent:GetFrameLevel() + 5); page:Hide(); self.pages[2] = page
    local title = page:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge"); title:SetPoint("TOPLEFT", page, "TOPLEFT", 18, -15); title:SetText("Second Screen")
    local help = page:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); help:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -8); help:SetText("These are WoW action slots 25-48, shown by ThorPad as slots 1-24.")
    self.secondScreenCells = {}
    for logical = 1, C.SECOND_SCREEN_SLOT_COUNT do
        local actionID = ThorPad.SecondScreen:GetActionID(logical); local cell = Widgets:CreateActionCell(page, "ThorPadSecondScreenButton" .. logical, 50, "native")
        local column, row = (logical - 1) % 8, math.floor((logical - 1) / 8); cell:SetPoint("TOPLEFT", page, "TOPLEFT", 20 + column * 62, -106 - row * 94)
        cell.logical, cell.actionID = logical, actionID; cell.label:SetText(logical); self.secondScreenCells[logical] = cell
        cell:SetScript("OnEnter", function(button) if HasAction(button.actionID) then showTooltip(button) end end); cell:SetScript("OnLeave", function() GameTooltip:Hide() end)
        cell:SetScript("OnDragStart", function(button) if not InCombatLockdown() and HasAction(button.actionID) then PickupAction(button.actionID) else if InCombatLockdown() then self:SetStatus("Leave combat to edit action slots.", true) end end end)
        local function place(button) if InCombatLockdown() then self:SetStatus("Leave combat to edit action slots.", true); return end; PlaceAction(button.actionID); self:SetStatus("Second-screen slot " .. button.logical .. " updated."); self:RefreshActions() end
        cell:SetScript("OnReceiveDrag", place)
        cell:SetScript("OnMouseUp", function(button, mouseButton)
            if mouseButton == "RightButton" then if InCombatLockdown() then self:SetStatus("Leave combat to edit action slots.", true) else PickupAction(button.actionID); ClearCursor(); self:RefreshActions() end
            elseif GetCursorInfo() then place(button) end
        end)
    end
end

function Settings:CreateDisplayPage(parent)
    local page = CreateFrame("Frame", nil, parent); page:SetAllPoints(parent); page:SetFrameLevel(parent:GetFrameLevel() + 5); page:Hide(); self.pages[3] = page
    local controller = Widgets:CreateSection(page, "Controller", 14, -14, 492, 128)
    self.controllerCheck = Widgets:CreateCheck(controller, "Enable Controller Support", 10, -31, function(value) ThorPadDB.controller.enabled = value; ThorPad.Display:Apply(); ThorPad.UINavigation:Reconcile(); self:RefreshDisplayPage() end)
    self.navigationCheck = Widgets:CreateCheck(controller, "Enable Controller UI Navigation", 10, -62, function(value) ThorPadDB.controller.uiNavigation = value; ThorPad.UINavigation:Reconcile(); self:RefreshDisplayPage() end)
    self.controllerNative = controller:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.controllerNative:SetPoint("TOPLEFT", controller, "TOPLEFT", 15, -98)
    self.glyphButton = CreateFrame("Button", nil, controller, "UIPanelButtonTemplate"); self.glyphButton:SetSize(180, 24); self.glyphButton:SetPoint("TOPRIGHT", controller, "TOPRIGHT", -14, -36)
    self.glyphButton:SetScript("OnClick", function()
        local nextFamily = { auto = "xbox", xbox = "playstation", playstation = "aynthor", aynthor = "auto" }
        ThorPadDB.controller.glyphFamily = nextFamily[ThorPadDB.controller.glyphFamily] or "auto"
        self:RefreshAll()
    end)

    local screen = Widgets:CreateSection(page, "Second Screen", 14, -152, 492, 104)
    self.screenCheck = Widgets:CreateCheck(screen, "Enable Second Screen", 10, -31, function(value) ThorPadDB.secondScreen.enabled = value; ThorPad.Display:Apply(); self:RefreshDisplayPage() end)
    self.reduceCheck = Widgets:CreateCheck(screen, "Enable Reduced In-Game UI", 10, -62, function(value) ThorPadDB.secondScreen.reduceUI = value; ThorPad.Display:Apply(); self:RefreshDisplayPage() end)

    local appearance = Widgets:CreateSection(page, "Appearance", 14, -266, 492, 76)
    local scaleDown = CreateFrame("Button", nil, appearance, "UIPanelButtonTemplate"); scaleDown:SetSize(28, 22); scaleDown:SetPoint("TOPLEFT", appearance, "TOPLEFT", 14, -35); scaleDown:SetText("-")
    self.scaleLabel = appearance:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); self.scaleLabel:SetPoint("LEFT", scaleDown, "RIGHT", 12, 0)
    local scaleUp = CreateFrame("Button", nil, appearance, "UIPanelButtonTemplate"); scaleUp:SetSize(28, 22); scaleUp:SetPoint("LEFT", self.scaleLabel, "RIGHT", 12, 0); scaleUp:SetText("+")
    local function changeScale(delta) ThorPadDB.display.actionScale = math.max(.75, math.min(1.35, ThorPadDB.display.actionScale + delta)); self:RefreshAll() end
    scaleDown:SetScript("OnClick", function() changeScale(-.05) end); scaleUp:SetScript("OnClick", function() changeScale(.05) end)
    local glyphDown = CreateFrame("Button", nil, appearance, "UIPanelButtonTemplate"); glyphDown:SetSize(28, 22); glyphDown:SetPoint("TOPLEFT", appearance, "TOPLEFT", 250, -35); glyphDown:SetText("-")
    self.glyphScaleLabel = appearance:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); self.glyphScaleLabel:SetPoint("LEFT", glyphDown, "RIGHT", 12, 0)
    local glyphUp = CreateFrame("Button", nil, appearance, "UIPanelButtonTemplate"); glyphUp:SetSize(28, 22); glyphUp:SetPoint("LEFT", self.glyphScaleLabel, "RIGHT", 12, 0); glyphUp:SetText("+")
    local function changeGlyphScale(delta) ThorPadDB.display.glyphScale = math.max(.75, math.min(1.35, ThorPadDB.display.glyphScale + delta)); self:RefreshAll() end
    glyphDown:SetScript("OnClick", function() changeGlyphScale(-.05) end); glyphUp:SetScript("OnClick", function() changeGlyphScale(.05) end)

    local connection = Widgets:CreateSection(page, "Connection", 14, -352, 492, 92)
    self.connectionStatus = connection:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); self.connectionStatus:SetPoint("TOPLEFT", connection, "TOPLEFT", 14, -34); self.connectionStatus:SetJustifyH("LEFT")
    self.connectionEndpoint = connection:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.connectionEndpoint:SetPoint("TOPLEFT", self.connectionStatus, "BOTTOMLEFT", 0, -6)
    self.connectionPairing = connection:CreateFontString(nil, "OVERLAY", "GameFontNormal"); self.connectionPairing:SetPoint("TOPLEFT", self.connectionEndpoint, "BOTTOMLEFT", 0, -6)
    self.regenerate = CreateFrame("Button", nil, connection, "UIPanelButtonTemplate"); self.regenerate:SetSize(160, 22); self.regenerate:SetPoint("BOTTOMRIGHT", connection, "BOTTOMRIGHT", -10, 8); self.regenerate:SetText("Regenerate Pairing Code")
    self.regenerate:SetScript("OnClick", function() if ThorPad.Bridge:RegeneratePairingCode() then self:SetStatus("Pairing code regenerated.") else self:SetStatus("Native bridge is unavailable.", true) end; self:RefreshBridge() end)
end

function Settings:Create()
    local frame = CreateFrame("Frame", "ThorPadConfig", UIParent, "UIPanelDialogTemplate"); frame:SetSize(732, 520); frame:SetPoint("CENTER"); frame:SetFrameStrata("DIALOG"); frame:SetToplevel(true); frame:SetMovable(true); frame:EnableMouse(true); frame:RegisterForDrag("LeftButton")
    frame:SetScript("OnDragStart", function(self) self:StartMoving() end); frame:SetScript("OnDragStop", function(self) self:StopMovingOrSizing() end); frame:Hide(); frame.title:SetText("ThorPad")
    tinsert(UISpecialFrames, "ThorPadConfig"); self.frame, self.pages, self.navigation = frame, {}, {}
    local nav = CreateFrame("Frame", nil, frame); nav:SetPoint("TOPLEFT", frame, "TOPLEFT", 18, -38); nav:SetSize(166, 452); Widgets:SetBackdrop(nav)
    local content = CreateFrame("Frame", nil, frame); content:SetPoint("TOPLEFT", nav, "TOPRIGHT", 10, 0); content:SetSize(520, 452); Widgets:SetBackdrop(content); self.content = content
    local labels = { "Controller Mapping", "Second Screen", "Display & Connection" }
    for index, label in ipairs(labels) do
        local button = CreateFrame("Button", nil, nav, "UIPanelButtonTemplate"); button:SetSize(148, 30); button:SetPoint("TOP", nav, "TOP", 0, -18 - (index - 1) * 40); button:SetText(label); button.index = index
        local buttonText = button:GetFontString(); buttonText:SetFont([[Fonts\FRIZQT__.TTF]], 11); buttonText:SetWidth(126); buttonText:SetJustifyH("CENTER")
        button:SetScript("OnClick", function(item) self:ShowPage(item.index) end); self.navigation[index] = button
    end
    self.status = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); self.status:SetPoint("BOTTOM", frame, "BOTTOM", 76, 14)
    local creators = { self.CreateControllerPage, self.CreateSecondScreenPage, self.CreateDisplayPage }
    for index, creator in ipairs(creators) do
        local ok, message = pcall(creator, self, content)
        if not ok then
            local page = self.pages[index]
            if not page then page = CreateFrame("Frame", nil, content); page:SetAllPoints(content); page:SetFrameLevel(content:GetFrameLevel() + 10); self.pages[index] = page end
            local errorText = page:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); errorText:SetPoint("TOPLEFT", page, "TOPLEFT", 24, -28); errorText:SetWidth(460); errorText:SetJustifyH("LEFT"); errorText:SetJustifyV("TOP")
            errorText:SetText("|cffff5050ThorPad could not build this page:|r\n" .. tostring(message))
            if DEFAULT_CHAT_FRAME then DEFAULT_CHAT_FRAME:AddMessage("|cffff5050ThorPad page error:|r " .. tostring(message)) end
        end
    end
    frame:SetScript("OnShow", function() self:RefreshAll() end); self:ShowPage(1)
end

function Settings:ShowPage(index)
    self.selectedPage = index
    for pageIndex = 1, 3 do
        local page = self.pages[pageIndex]
        if page then
            if pageIndex == index then page:SetFrameLevel(self.content:GetFrameLevel() + 5); page:Show() else page:Hide() end
        end
    end
    for buttonIndex, button in ipairs(self.navigation) do if buttonIndex == index then button:LockHighlight() else button:UnlockHighlight() end end
    self:RefreshAll()
end
function Settings:RefreshControllerPage()
    if not self.controllerCells then return end
    for _, tab in ipairs(self.layerTabs) do if tab.layer == self.selectedLayer then tab:LockHighlight() else tab:UnlockHighlight() end end
    for _, cell in ipairs(self.controllerCells) do cell.layer = self.selectedLayer; Widgets:RefreshControllerCell(cell) end
end
function Settings:RefreshSecondScreenPage() if self.secondScreenCells then for _, cell in ipairs(self.secondScreenCells) do Widgets:RefreshNativeCell(cell) end end end
function Settings:RefreshDisplayPage()
    if not self.controllerCheck then return end
    self.controllerCheck:SetChecked(ThorPadDB.controller.enabled); self.navigationCheck:SetChecked(ThorPadDB.controller.uiNavigation); self.screenCheck:SetChecked(ThorPadDB.secondScreen.enabled); self.reduceCheck:SetChecked(ThorPadDB.secondScreen.reduceUI)
    if ThorPadDB.controller.enabled and ThorPad.Native:IsUINavigationAvailable() then self.navigationCheck:Enable() else self.navigationCheck:Disable() end
    self.controllerNative:SetText(ThorPad.Native:IsControllerAvailable() and "|cff55ff55Native controller integration available|r" or "|cff888888Native controller integration unavailable|r")
    if ThorPadDB.controller.enabled and ThorPadDB.secondScreen.enabled then self.reduceCheck:Enable() else self.reduceCheck:Disable() end
    local familyLabels = { auto = "Auto", xbox = "Xbox", playstation = "PlayStation", aynthor = "AYN Thor" }
    if WXLGamepadConfiguredGlyphStyle and WXLGamepadConfiguredGlyphStyle ~= "Auto" then
        self.glyphButton:SetText("Glyphs: " .. WXLGamepadConfiguredGlyphStyle .. " (config)"); self.glyphButton:Disable()
    else self.glyphButton:SetText("Glyphs: " .. (familyLabels[ThorPadDB.controller.glyphFamily] or "Auto")); self.glyphButton:Enable() end
    self.scaleLabel:SetText(string.format("Action: %d%%", math.floor(ThorPadDB.display.actionScale * 100 + .5))); self.glyphScaleLabel:SetText(string.format("Glyph: %d%%", math.floor(ThorPadDB.display.glyphScale * 100 + .5))); self:RefreshBridge()
end
function Settings:RefreshBridge()
    if not self.connectionStatus then return end
    local state = ThorPad.Bridge:GetStatus()
    if not state.available then self.connectionStatus:SetText("|cff888888Unavailable|r"); self.connectionEndpoint:SetText("Host: unavailable    Port: " .. C.DEFAULT_PORT); self.connectionPairing:SetText("Pairing code: unavailable"); self.regenerate:Disable(); return end
    local status = state.connected and "|cff55ff55Connected|r" or (state.listening and "|cffffd040Waiting for connection|r" or "|cffff5050Disconnected|r")
    self.connectionStatus:SetText(status); self.connectionEndpoint:SetText(string.format("Host: %s    Port: %d", state.bindAddress ~= "" and state.bindAddress or "0.0.0.0", (state.port and state.port > 0) and state.port or C.DEFAULT_PORT))
    self.connectionPairing:SetText(state.paired and ("Paired: " .. (state.device ~= "" and state.device or "device")) or ("Pairing code: " .. (state.pairingCode ~= "" and state.pairingCode or "unavailable"))); self.regenerate:Enable()
end
function Settings:RefreshActions() self:RefreshControllerPage(); self:RefreshSecondScreenPage(); ThorPad.ActionOverlay:Refresh() end
function Settings:RefreshAll() self:RefreshActions(); self:RefreshDisplayPage() end
function Settings:Toggle() if self.frame:IsShown() then self.frame:Hide() else self.frame:Show() end end
