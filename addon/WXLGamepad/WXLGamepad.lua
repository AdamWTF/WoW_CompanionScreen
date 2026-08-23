local addonName = ...
local labels = { "X", "O", "[]", "/\\", "UP", "DOWN", "LEFT", "RIGHT" }
local slots = { 1, 2, 3, 4, 5, 6, 7, 8 }
local positions = {
    {390, -96}, {432, -54}, {348, -54}, {390, -12}, -- face: cross, circle, square, triangle
    {80, -12}, {80, -96}, {38, -54}, {122, -54},   -- D-pad: up, down, left, right
}

local panel = CreateFrame("Frame", "WXLGamepadActionOverlay", UIParent)
panel:SetSize(480, 144)
panel:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, 42)
panel:SetFrameStrata("MEDIUM")

local background = panel:CreateTexture(nil, "BACKGROUND")
background:SetAllPoints(panel)
background:SetTexture(0, 0, 0, .62)

local title = panel:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
title:SetPoint("TOP", panel, "TOP", 0, -5)
title:SetText("WXL GAMEPAD")

local modifiers = {}
for index, name in ipairs({ "L1", "L2", "R1", "R2" }) do
    local text = panel:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    text:SetPoint("TOPLEFT", panel, "TOPLEFT", 154 + (index - 1) * 47, -22)
    text:SetText(name)
    modifiers[index] = text
end

local buttons = {}
for index = 1, 8 do
    local button = CreateFrame("Frame", nil, panel)
    button:SetSize(38, 38)
    button:SetPoint("TOPLEFT", panel, "TOPLEFT", positions[index][1], positions[index][2])
    local icon = button:CreateTexture(nil, "ARTWORK")
    icon:SetAllPoints(button)
    icon:SetTexture("Interface\\Icons\\INV_Misc_QuestionMark")
    button.icon = icon
    local border = button:CreateTexture(nil, "OVERLAY")
    border:SetTexture("Interface\\Buttons\\UI-Quickslot2")
    border:SetAllPoints(button)
    button.border = border
    local glyph = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    glyph:SetPoint("TOPLEFT", button, "TOPLEFT", 2, -2)
    glyph:SetText(labels[index])
    button.glyph = glyph
    local count = button:CreateFontString(nil, "OVERLAY", "NumberFontNormalSmall")
    count:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -2, 2)
    button.count = count
    local cooldown = CreateFrame("Cooldown", nil, button, "CooldownFrameTemplate")
    cooldown:SetAllPoints(button)
    button.cooldown = cooldown
    buttons[index] = button
end

local function hideDefaultBar()
    if not MainMenuBar then return end
    MainMenuBar:Hide()
    MainMenuBar:SetScript("OnShow", function(self) self:Hide() end)
end

local function layer()
    if type(WXLGamepadNativeLayer) == "number" then return WXLGamepadNativeLayer end
    if type(WXLGamepad_GetState) ~= "function" then return 1 end
    local active = WXLGamepad_GetState()
    return tonumber(active) or 1
end

local function refresh()
    local active = layer()
    if active < 1 or active > 5 then active = 1 end
    for index = 1, 4 do
        if index + 1 == active then modifiers[index]:SetTextColor(1, .82, .15)
        else modifiers[index]:SetTextColor(.55, .55, .55) end
    end
    for index = 1, 8 do
        local slot = (active - 1) * 8 + index
        local button = buttons[index]
        local texture = GetActionTexture(slot)
        button.icon:SetTexture(texture or "Interface\\Icons\\INV_Misc_QuestionMark")
        local start, duration, enabled = GetActionCooldown(slot)
        if enabled and duration and duration > 0 then button.cooldown:SetCooldown(start, duration) else button.cooldown:Hide() end
        local count = GetActionCount(slot)
        button.count:SetText(count and count > 0 and count or "")
        local usable, noMana = IsUsableAction(slot)
        if usable then button.icon:SetVertexColor(1, 1, 1)
        elseif noMana then button.icon:SetVertexColor(.45, .45, 1)
        else button.icon:SetVertexColor(.4, .4, .4) end
        button.glyph:SetText(labels[index] .. " " .. slot)
    end
end

panel:RegisterEvent("PLAYER_LOGIN")
panel:RegisterEvent("ACTIONBAR_SLOT_CHANGED")
panel:RegisterEvent("ACTIONBAR_UPDATE_COOLDOWN")
panel:RegisterEvent("ACTIONBAR_UPDATE_USABLE")
panel:RegisterEvent("ACTIONBAR_UPDATE_STATE")
panel:SetScript("OnEvent", function()
    hideDefaultBar()
    refresh()
end)
panel:SetScript("OnUpdate", function(self, elapsed)
    self.elapsed = (self.elapsed or 0) + elapsed
    if self.elapsed < .08 then return end
    self.elapsed = 0
    refresh()
end)

SLASH_WXLGAMEPAD1 = "/wxlgamepad"
SlashCmdList.WXLGAMEPAD = function()
    DEFAULT_CHAT_FRAME:AddMessage("WXLGamepad: fixed slots Base 1-8, L1 9-16, L2 17-24, R1 25-32, R2 33-40. Touchpad sensitivity: 0.8.")
end
