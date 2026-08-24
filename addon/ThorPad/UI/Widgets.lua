local addonName, ThorPad = ...
local Config = ThorPad.Config

ThorPad.UI = ThorPad.UI or {}
ThorPad.UI.Widgets = {}

function ThorPad.UI.Widgets:SetBackdrop(frame)
    frame:SetBackdrop({ bgFile = [[Interface\Tooltips\UI-Tooltip-Background]], edgeFile = [[Interface\Tooltips\UI-Tooltip-Border]], tile = true, tileSize = 16, edgeSize = 16, insets = { left = 4, right = 4, top = 4, bottom = 4 } })
    frame:SetBackdropColor(0.05, 0.05, 0.05, 0.94)
    frame:SetBackdropBorderColor(0.62, 0.48, 0.18, 1)
end

function ThorPad.UI.Widgets:CreateActionCell(parent, x, y, size, interactive)
    local cell = CreateFrame("Button", nil, parent)
    cell:SetSize(size, size)
    cell:SetPoint("TOPLEFT", parent, "TOPLEFT", x, y)
    cell:EnableMouse(interactive)
    if interactive then cell:RegisterForDrag("LeftButton"); cell:RegisterForClicks("AnyUp") end

    local iconInset = math.floor(size * 0.13)

    local background = cell:CreateTexture(nil, "BACKGROUND")
    background:SetTexture([[Interface\Minimap\UI-Minimap-Background]])
    background:SetAllPoints(cell)
    background:SetVertexColor(0.08, 0.08, 0.08, 1)

    local icon = cell:CreateTexture(nil, "ARTWORK")
    icon:SetPoint("TOPLEFT", cell, "TOPLEFT", iconInset, -iconInset)
    icon:SetPoint("BOTTOMRIGHT", cell, "BOTTOMRIGHT", -iconInset, iconInset)
    icon:SetTexCoord(0.08, 0.92, 0.08, 0.92)
    cell.icon = icon

    local cooldown = CreateFrame("Cooldown", nil, cell, "CooldownFrameTemplate")
    cooldown:SetPoint("TOPLEFT", icon, "TOPLEFT", 0, 0)
    cooldown:SetPoint("BOTTOMRIGHT", icon, "BOTTOMRIGHT", 0, 0)
    cell.cooldown = cooldown

    local disabled = cell:CreateTexture(nil, "OVERLAY")
    disabled:SetTexture([[Interface\Minimap\UI-Minimap-Background]])
    disabled:SetAllPoints(cell)
    disabled:SetVertexColor(0, 0, 0, 0.72)
    cell.disabled = disabled


    -- Wrath has no texture masks. A built-in circular background, inset icon,
    -- and action-button ring provide the round presentation without newer APIs.
    local ringFrame = CreateFrame("Frame", nil, cell)
    ringFrame:SetAllPoints(cell)
    ringFrame:SetFrameStrata(cell:GetFrameStrata())
    ringFrame:SetFrameLevel(cell:GetFrameLevel() + 10)

    local ring = ringFrame:CreateTexture(nil, "OVERLAY")
    ring:SetTexture([[Interface\Buttons\UI-ActionButton-Border]])
    ring:SetPoint("CENTER", ringFrame, "CENTER", 0, 0)
    ring:SetSize(size + 12, size + 12)
    ring:SetBlendMode("ADD")
    ring:SetVertexColor(0.72, 0.62, 0.36, 0.78)
    cell.ring = ring

    local glyphSize = math.max(24, math.floor(size * Config.glyphScale))
    local glyphFrame = CreateFrame("Frame", nil, cell)
    glyphFrame:SetSize(glyphSize, glyphSize)
    glyphFrame:SetPoint("TOP", cell, "TOP", 0, math.floor(size * 0.13))
    glyphFrame:SetFrameStrata(cell:GetFrameStrata())
    glyphFrame:SetFrameLevel(cell:GetFrameLevel() + 20)
    local glyph = glyphFrame:CreateTexture(nil, "OVERLAY")
    glyph:SetAllPoints(glyphFrame)
    cell.glyphFrame, cell.controllerGlyph = glyphFrame, glyph

    local count = cell:CreateFontString(nil, "OVERLAY", "NumberFontNormalSmall")
    count:SetPoint("BOTTOMRIGHT", cell, "BOTTOMRIGHT", -5, 5)
    count:SetShadowOffset(1, -1)
    cell.count = count

    function cell:SetAction(slot) self.slot = slot end
    function cell:SetControllerGlyph(index)
        local name = Config.glyphs[index]
        if name and self.slot and self.slot > 0 then self.controllerGlyph:SetTexture(Config.mediaPath .. name); self.glyphFrame:Show() else self.glyphFrame:Hide() end
    end
    function cell:SetDisabled(disabled) if disabled then self.disabled:Show() else self.disabled:Hide() end end
    function cell:Refresh(index)
        local slot = self.slot
        self:SetControllerGlyph(index)
        if not slot or slot == 0 then
            self.icon:SetTexture([[Interface\Icons\INV_Misc_QuestionMark]]); self.icon:SetVertexColor(0.32, 0.32, 0.32)
            self.count:SetText(""); self.cooldown:Hide(); self:SetDisabled(true); return
        end
        self:SetDisabled(false)
        self.icon:SetTexture(GetActionTexture(slot) or [[Interface\Icons\INV_Misc_QuestionMark]])
        local usable, noMana = IsUsableAction(slot)
        if usable then self.icon:SetVertexColor(1, 1, 1) elseif noMana then self.icon:SetVertexColor(0.48, 0.48, 1) else self.icon:SetVertexColor(0.42, 0.42, 0.42) end
        local countValue = GetActionCount(slot)
        self.count:SetText(countValue and countValue > 0 and countValue or "")
        local start, duration, enabled = GetActionCooldown(slot)
        if enabled and duration and duration > 0 then self.cooldown:Show(); self.cooldown:SetCooldown(start, duration) else self.cooldown:Hide() end
    end
    return cell
end
