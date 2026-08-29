local addonName, WCS = ...

WCS.UINavigation = { candidates = {}, roots = {}, active = false, elapsed = 0 }
local Navigation = WCS.UINavigation

local function shown(frame) return frame and frame.IsShown and frame:IsShown() end
local function enabled(frame)
    if not frame.IsEnabled then return true end
    local value = frame:IsEnabled(); return value ~= false and value ~= nil and value ~= 0
end
local function frameLevel(frame) return frame and frame.GetFrameLevel and frame:GetFrameLevel() or 0 end
local function named(name) local frame = _G[name]; return shown(frame) and frame or nil end

local function bounds(frame)
    if not frame or not frame.GetCenter then return nil end
    local x, y = frame:GetCenter(); local width, height = frame:GetWidth() or 0, frame:GetHeight() or 0
    if not x or not y or width <= 0 or height <= 0 then return nil end
    local parentWidth, parentHeight = UIParent:GetWidth() or 0, UIParent:GetHeight() or 0
    if parentWidth <= 0 or parentHeight <= 0 or x + width / 2 < 0 or y + height / 2 < 0 or x - width / 2 > parentWidth or y - height / 2 > parentHeight then return nil end
    return x, y, width, height
end

local function interactive(frame)
    if not frame or frame.WCSIgnoreNavigation or not frame.IsVisible or not frame:IsVisible() or not enabled(frame) then return false end
    local object = frame.GetObjectType and frame:GetObjectType()
    if object ~= "Button" and object ~= "CheckButton" then return false end
    if frame.IsMouseEnabled and not frame:IsMouseEnabled() then return false end
    local alpha = frame.GetEffectiveAlpha and frame:GetEffectiveAlpha() or (frame.GetAlpha and frame:GetAlpha()) or 1
    return alpha > .05 and bounds(frame) ~= nil
end

local function isBagItem(root, frame)
    if not root or not frame or not frame.GetID or not GetContainerItemInfo then return false end
    local slot, bag = frame:GetID(), root:GetID()
    if not slot or slot <= 0 or bag == nil then return false end
    return GetContainerItemInfo(bag, slot) ~= nil
end

local function isBagClose(frame)
    local name = frame and frame.GetName and frame:GetName()
    return type(name) == "string" and name:find("CloseButton", 1, true) ~= nil
end

local function collectChildren(root, frame, kind, output, visited)
    if not frame or visited[frame] then return end
    visited[frame] = true
    if interactive(frame) then
        local include, candidateKind = true, "standard"
        if kind == "bags" then
            if isBagItem(root, frame) then candidateKind = "bag_item"
            elseif not isBagClose(frame) then include = false end
        end
        if include then
            local x, y, width, height = bounds(frame)
            output[#output + 1] = { node = frame, root = root, rootKind = kind, kind = candidateKind, x = x, y = y, width = width, height = height, order = #output + 1 }
        end
    end
    if frame.GetChildren then
        local children = { frame:GetChildren() }
        for _, child in ipairs(children) do collectChildren(root, child, kind, output, visited) end
    end
end

local function addRoot(roots, frame, kind, priority)
    if shown(frame) then roots[#roots + 1] = { frame = frame, kind = kind, priority = priority } end
end

function Navigation:GetVisibleRoots()
    local popups = {}
    for index = 1, 4 do addRoot(popups, named("StaticPopup" .. index), "popup", 100) end
    if #popups > 0 then
        table.sort(popups, function(a, b) return frameLevel(a.frame) > frameLevel(b.frame) end)
        return { popups[1] }
    end
    local menu = named("GameMenuFrame")
    if menu then return { { frame = menu, kind = "game_menu", priority = 90 } } end
    local roots = {}
    addRoot(roots, named("QuestFrame"), "quest", 60); addRoot(roots, named("GossipFrame"), "gossip", 60)
    addRoot(roots, named("MerchantFrame"), "merchant", 50); addRoot(roots, named("WCSConfig"), "wcs", 45)
    for index = 1, 13 do addRoot(roots, named("ContainerFrame" .. index), "bags", 30) end
    table.sort(roots, function(a, b) if a.priority == b.priority then return frameLevel(a.frame) > frameLevel(b.frame) end return a.priority > b.priority end)
    return roots
end

function Navigation:Scan()
    self.roots, self.candidates = self:GetVisibleRoots(), {}
    local visited = {}
    for _, descriptor in ipairs(self.roots) do collectChildren(descriptor.frame, descriptor.frame, descriptor.kind, self.candidates, visited) end
    return self.candidates
end

function Navigation:CandidateFor(node)
    if not node then return nil end
    for _, candidate in ipairs(self.candidates) do if candidate.node == node then return candidate end end
    return nil
end

function Navigation:IsCandidateValid(candidate)
    return candidate and candidate.node and interactive(candidate.node) and self:CandidateFor(candidate.node) ~= nil
end

function Navigation:TopLeft(candidates)
    local best
    for _, candidate in ipairs(candidates) do
        if not best or candidate.y > best.y or (candidate.y == best.y and candidate.x < best.x) then best = candidate end
    end
    return best
end

function Navigation:NamedCandidate(names)
    for _, name in ipairs(names) do
        local node = _G[name]; local candidate = self:CandidateFor(node)
        if candidate then return candidate end
    end
end

function Navigation:ChooseDefault()
    local root = self.roots[1]
    if not root then return nil end
    if root.kind == "popup" then
        local name = root.frame:GetName(); local candidate = name and self:NamedCandidate({ name .. "Button1" })
        if candidate then return candidate end
    elseif root.kind == "game_menu" then
        local candidate = self:NamedCandidate({ "GameMenuButtonContinue", "GameMenuButtonResume" }); if candidate then return candidate end
    elseif root.kind == "quest" then
        local candidate = self:NamedCandidate({ "QuestFrameAcceptButton", "QuestFrameCompleteQuestButton", "QuestFrameCompleteButton", "QuestFrameGoodbyeButton" }); if candidate then return candidate end
    elseif root.kind == "gossip" then
        local candidate = self:NamedCandidate({ "GossipTitleButton1", "GossipFrameGreetingPanelGoodbyeButton" }); if candidate then return candidate end
    elseif root.kind == "merchant" then
        local candidate = self:NamedCandidate({ "MerchantItem1ItemButton", "MerchantItem1" }); if candidate then return candidate end
    elseif root.kind == "wcs" and WCS.Settings and WCS.Settings.navigation then
        local candidate = self:CandidateFor(WCS.Settings.navigation[1]); if candidate then return candidate end
    end
    for _, candidate in ipairs(self.candidates) do if candidate.kind == "bag_item" then return candidate end end
    return self:TopLeft(self.candidates)
end

function Navigation:FindDirectional(candidates, current, direction)
    if not current then return self:TopLeft(candidates) end
    local best, bestScore
    for _, candidate in ipairs(candidates) do
        if candidate ~= current and not candidate.disabled then
            local dx, dy = candidate.x - current.x, candidate.y - current.y
            local primary, perpendicular
            if direction == "right" then primary, perpendicular = dx, math.abs(dy)
            elseif direction == "left" then primary, perpendicular = -dx, math.abs(dy)
            elseif direction == "up" then primary, perpendicular = dy, math.abs(dx)
            elseif direction == "down" then primary, perpendicular = -dy, math.abs(dx) end
            if primary and primary > .5 then
                local score = primary + perpendicular * 2
                if not bestScore or score < bestScore or (score == bestScore and candidate.order < best.order) then best, bestScore = candidate, score end
            end
        end
    end
    return best
end

function Navigation:EnsureHighlight()
    if self.highlight then return end
    local frame = CreateFrame("Frame", "WCSUINavigationFocus", UIParent); frame:SetFrameStrata("TOOLTIP"); frame:EnableMouse(false); frame:Hide()
    frame:SetBackdrop({ edgeFile = [[Interface\Tooltips\UI-Tooltip-Border]], edgeSize = 14 }); frame:SetBackdropBorderColor(1, .78, .22, 1)
    self.highlight = frame
end

function Navigation:SetSelected(candidate, movePointer)
    self.selected = candidate; self:EnsureHighlight()
    if not candidate or not candidate.node then self.highlight:Hide(); return end
    self.highlight:ClearAllPoints(); self.highlight:SetPoint("TOPLEFT", candidate.node, "TOPLEFT", -5, 5); self.highlight:SetPoint("BOTTOMRIGHT", candidate.node, "BOTTOMRIGHT", 5, -5); self.highlight:Show()
    if movePointer then
        local width, height = UIParent:GetWidth() or 0, UIParent:GetHeight() or 0
        if width > 0 and height > 0 then WCS.Native:MoveUINavigationPointer(candidate.x / width, candidate.y / height) end
    end
end

function Navigation:SetActive(active)
    active = active and true or false
    if self.active == active then return end
    if not WCS.Native:SetUINavigationActive(active) and active then active = false end
    self.active = active
    if not active then self.selected = nil; if self.highlight then self.highlight:Hide() end end
end

function Navigation:ShouldActivate()
    return WCSDB and WCSDB.controller and WCSDB.controller.enabled and WCSDB.controller.uiNavigation and not InCombatLockdown() and WCS.Native:IsUINavigationAvailable() and #self.roots > 0 and #self.candidates > 0
end

function Navigation:Reconcile()
    self:Scan(); local desired = self:ShouldActivate(); self:SetActive(desired)
    if not desired then return end
    local previousNode = self.selected and self.selected.node
    local selected = previousNode and self:CandidateFor(previousNode)
    if not selected then selected = self:ChooseDefault() end
    if not previousNode or not selected or selected.node ~= previousNode then self:SetSelected(selected, true) else self.selected = selected end
end

function Navigation:SyncMouseFocus()
    if not self.active or not GetMouseFocus then return end
    local focus = GetMouseFocus()
    while focus do
        local candidate = self:CandidateFor(focus)
        if candidate then if not self.selected or self.selected.node ~= candidate.node then self:SetSelected(candidate, false) end; return end
        focus = focus.GetParent and focus:GetParent() or nil
    end
end

function Navigation:Handle(command)
    if not self.active then return end
    local oldNode = self.selected and self.selected.node
    self:Scan()
    local current = oldNode and self:CandidateFor(oldNode) or nil
    if command == "back" then return end
    if command == "confirm" then
        if not current then self:SetSelected(self:ChooseDefault(), true); return end
        self:SetSelected(current, true); WCS.Native:ClickUINavigationPointer(current.kind == "bag_item" and "right" or "left"); return
    end
    if command == "up" or command == "down" or command == "left" or command == "right" then
        local target = self:FindDirectional(self.candidates, current, command)
        if target then self:SetSelected(target, true) elseif current then self:SetSelected(current, false) else self:SetSelected(self:ChooseDefault(), true) end
    end
end

function Navigation:Initialize() self:EnsureHighlight(); self:Reconcile() end
function Navigation:OnEvent(event)
    if event == "PLAYER_REGEN_DISABLED" then self:SetActive(false)
    elseif event == "PLAYER_REGEN_ENABLED" or event == "PLAYER_ENTERING_WORLD" then self:Reconcile() end
end
function Navigation:Tick(elapsed)
    self.elapsed = self.elapsed + elapsed
    if self.elapsed < .1 then return end
    self.elapsed = 0; self:Reconcile(); self:SyncMouseFocus()
end
