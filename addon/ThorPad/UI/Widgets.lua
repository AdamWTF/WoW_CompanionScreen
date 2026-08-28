local addonName, ThorPad = ...

ThorPad.UI = ThorPad.UI or {}
ThorPad.UI.Widgets = {}
local Widgets = ThorPad.UI.Widgets

function Widgets:SetBackdrop(frame)
    frame:SetBackdrop({ bgFile = [[Interface\Tooltips\UI-Tooltip-Background]], edgeFile = [[Interface\Tooltips\UI-Tooltip-Border]], tile = true, tileSize = 16, edgeSize = 16, insets = { left = 4, right = 4, top = 4, bottom = 4 } })
    frame:SetBackdropColor(.035, .035, .035, .96); frame:SetBackdropBorderColor(.62, .48, .18, 1)
end

function Widgets:CreateSection(parent, title, x, y, width, height)
    local frame = CreateFrame("Frame", nil, parent); frame:SetPoint("TOPLEFT", parent, "TOPLEFT", x, y); frame:SetSize(width, height); frame:SetFrameLevel(parent:GetFrameLevel() + 2); self:SetBackdrop(frame)
    local label = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal"); label:SetPoint("TOPLEFT", frame, "TOPLEFT", 12, -10); label:SetText(title)
    return frame
end

function Widgets:CreateCheck(parent, label, x, y, callback)
    local check = CreateFrame("CheckButton", nil, parent, "UICheckButtonTemplate"); check:SetPoint("TOPLEFT", parent, "TOPLEFT", x, y); check:SetSize(24, 24); check:SetFrameLevel(parent:GetFrameLevel() + 2)
    local labelText = check:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); labelText:SetPoint("LEFT", check, "RIGHT", 3, 1); labelText:SetText(label)
    check:SetScript("OnClick", function(self) callback(self:GetChecked() and true or false) end)
    return check
end

function Widgets:CreateActionCell(parent, name, size, kind)
    local cell = CreateFrame("Button", name, parent); cell:SetSize(size, size); cell:SetFrameLevel(parent:GetFrameLevel() + 2); cell:RegisterForDrag("LeftButton"); cell:RegisterForClicks("AnyUp")
    local background = cell:CreateTexture(nil, "BACKGROUND"); background:SetTexture([[Interface\Minimap\UI-Minimap-Background]]); background:SetAllPoints(cell); background:SetVertexColor(.07, .07, .07, 1)
    local inset = math.floor(size * .12)
    cell.icon = cell:CreateTexture(nil, "ARTWORK"); cell.icon:SetPoint("TOPLEFT", cell, "TOPLEFT", inset, -inset); cell.icon:SetPoint("BOTTOMRIGHT", cell, "BOTTOMRIGHT", -inset, inset); cell.icon:SetTexCoord(.08, .92, .08, .92)
    cell.cooldown = CreateFrame("Cooldown", nil, cell, "CooldownFrameTemplate"); cell.cooldown:SetAllPoints(cell.icon)
    cell.border = cell:CreateTexture(nil, "OVERLAY"); cell.border:SetTexture([[Interface\Buttons\UI-ActionButton-Border]]); cell.border:SetPoint("CENTER"); cell.border:SetSize(size + 12, size + 12); cell.border:SetBlendMode("ADD"); cell.border:SetVertexColor(.72, .62, .36, .82)
    cell.count = cell:CreateFontString(nil, "OVERLAY", "NumberFontNormalSmall"); cell.count:SetPoint("BOTTOMRIGHT", cell, "BOTTOMRIGHT", -5, 5)
    cell.actionLabel = cell:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    if kind == "display" then cell.actionLabel:SetPoint("TOP", cell, "TOP", 0, -5) else cell.actionLabel:SetPoint("BOTTOM", cell, "BOTTOM", 0, 5) end
    cell.actionLabel:SetWidth(size - 6); cell.actionLabel:SetJustifyH("CENTER")
    cell.label = cell:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); cell.label:SetPoint("TOP", cell, "BOTTOM", 0, -2)
    local glyphFrame = CreateFrame("Frame", nil, cell); glyphFrame:SetSize(25, 25)
    if kind == "display" then glyphFrame:SetPoint("BOTTOM", cell, "BOTTOM", 0, -8) else glyphFrame:SetPoint("TOP", cell, "TOP", 0, 8) end
    glyphFrame:SetFrameLevel(cell:GetFrameLevel() + 20)
    cell.glyph = glyphFrame:CreateTexture(nil, "OVERLAY"); cell.glyph:SetAllPoints(glyphFrame)
    cell.glyphText = glyphFrame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall"); cell.glyphText:SetPoint("CENTER"); cell.glyphFrame = glyphFrame
    return cell
end

function Widgets:SetGlyph(cell, control)
    local texture, label, red, green, blue = ThorPad.Glyphs:Get(control)
    cell.glyph:SetTexture(nil)
    if texture then cell.glyph:SetTexture(texture); cell.glyph:SetTexCoord(0, 1, 0, 1); cell.glyph:Show() else cell.glyph:Hide() end
    cell.glyphText:SetText(texture and "" or label or "?"); cell.glyphText:SetTextColor(red or 1, green or 1, blue or 1)
    cell.glyphFrame:SetScale((ThorPadDB.display.glyphScale or 1) * .75)
end

function Widgets:SetVisualState(cell, icon, count, usable, noResource, start, duration, enabled, current, inRange)
    cell.icon:SetTexture(icon or [[Interface\Icons\INV_Misc_QuestionMark]])
    if not icon then cell.icon:SetVertexColor(.28, .28, .28)
    elseif inRange == 0 then cell.icon:SetVertexColor(1, .18, .18)
    elseif usable then cell.icon:SetVertexColor(1, 1, 1)
    elseif noResource then cell.icon:SetVertexColor(.45, .45, 1)
    else cell.icon:SetVertexColor(.4, .4, .4) end
    cell.count:SetText(count and count > 0 and count or "")
    if duration and duration > 0 and enabled ~= 0 then cell.cooldown:Show(); cell.cooldown:SetCooldown(start or 0, duration) else cell.cooldown:Hide() end
    if current then cell.border:SetVertexColor(.15, 1, .25, 1) else cell.border:SetVertexColor(.72, .62, .36, .82) end
end

function Widgets:RefreshControllerCell(cell)
    local descriptor = ThorPad.Controller:GetAssignment(cell.layer, cell.control); local state = ThorPad.Controller:GetState(descriptor); self:SetGlyph(cell, cell.control)
    cell.actionLabel:SetText(descriptor and descriptor.type == "system" and descriptor.name or "")
    if state then self:SetVisualState(cell, state.icon, state.count, state.usable, state.noResource, state.start, state.duration, state.enabled, state.current, state.inRange)
    else self:SetVisualState(cell, nil, 0, false, false, 0, 0, 0, false, nil) end
end

function Widgets:RefreshNativeCell(cell)
    local slot = cell.actionID; local usable, noResource = IsUsableAction(slot); local start, duration, enabled = GetActionCooldown(slot); local inRange = IsActionInRange(slot)
    self:SetVisualState(cell, GetActionTexture(slot), GetActionCount(slot), usable, noResource, start, duration, enabled, IsCurrentAction(slot), inRange)
end
