local addonName, WCS = ...

WCS.Bridge = {}
local Bridge = WCS.Bridge
local Null = {}
local nativeSlots = {}
for logical = 1, WCS.Constants.SECOND_SCREEN_SLOT_COUNT do nativeSlots[logical] = WCS.SecondScreen:GetActionID(logical) end

local function escape(value)
    return value:gsub('[%z\1-\31\\"]', function(c)
        local replacements = { ['"'] = '\\"', ['\\'] = '\\\\', ['\b'] = '\\b', ['\f'] = '\\f', ['\n'] = '\\n', ['\r'] = '\\r', ['\t'] = '\\t' }
        return replacements[c] or string.format('\\u%04x', string.byte(c))
    end)
end

local function isArray(value)
    local count, maximum = 0, 0
    for key in pairs(value) do
        if type(key) ~= "number" or key < 1 or key ~= math.floor(key) then return false end
        count, maximum = count + 1, math.max(maximum, key)
    end
    return count == maximum
end

local function encode(value, stack)
    if value == Null or value == nil then return "null" end
    local kind = type(value)
    if kind == "boolean" then return value and "true" or "false" end
    if kind == "number" then return tostring(value) end
    if kind == "string" then return '"' .. escape(value) .. '"' end
    if kind ~= "table" then return "null" end
    stack = stack or {}; if stack[value] then error("cyclic JSON table") end; stack[value] = true
    local parts = {}
    if isArray(value) then
        for index = 1, #value do parts[index] = encode(value[index], stack) end
        stack[value] = nil; return "[" .. table.concat(parts, ",") .. "]"
    end
    for key, child in pairs(value) do table.insert(parts, '"' .. escape(tostring(key)) .. '":' .. encode(child, stack)) end
    table.sort(parts); stack[value] = nil; return "{" .. table.concat(parts, ",") .. "}"
end

local function playerState()
    local level = UnitLevel("player") or 0
    local current, required, rested = UnitXP("player") or 0, UnitXPMax("player") or 0, (GetXPExhaustion and GetXPExhaustion()) or 0
    local total, free = 0, 0
    for bag = 0, 4 do total = total + (GetContainerNumSlots(bag) or 0); free = free + (GetContainerNumFreeSlots(bag) or 0) end
    return {
        name = UnitName("player") or "", level = level, money = GetMoney() or 0,
        experience = { level = level, current = required > 0 and current or 0, required = required, rested = required > 0 and rested or 0, capped = required <= 0 or level >= 80 },
        bags = { used = total - free, total = total, free = free },
    }
end

local function actionName(kind, id)
    if not id then return nil end
    if kind == "spell" and GetSpellInfo then return GetSpellInfo(id) end
    if kind == "item" and GetItemInfo then return GetItemInfo(id) end
    if kind == "macro" and GetMacroInfo then return GetMacroInfo(id) end
    return nil
end

local function actionState(logical)
    local slot = nativeSlots[logical]
    if not HasAction(slot) then return { slot = logical, empty = true } end
    local kind, id, subtype = GetActionInfo(slot)
    local usable, noResource = IsUsableAction(slot)
    local inRange = IsActionInRange(slot)
    local start, duration, enabled = GetActionCooldown(slot)
    start, duration = start or 0, duration or 0
    local remaining = math.max(0, (start + duration - GetTime()) * 1000)
    return {
        slot = logical, empty = false, kind = kind or subtype or "unknown", id = id or 0,
        name = actionName(kind, id) or GetActionText(slot) or "", icon = GetActionTexture(slot) or "",
        text = GetActionText(slot) or "", count = GetActionCount(slot) or 0,
        usable = usable and true or false, insufficientResource = noResource and true or false,
        inRange = inRange == nil and Null or (inRange == 1), current = IsCurrentAction(slot) and true or false,
        equipped = IsEquippedAction(slot) and true or false,
        cooldown = { active = enabled == 1 and duration > 0 and remaining > 0, durationMs = math.floor(duration * 1000 + 0.5), remainingMs = math.max(0, math.floor(remaining / 1000 + 0.5) * 1000) },
    }
end

local function allActions()
    local slots = {}; for logical = 1, WCS.Constants.SECOND_SCREEN_SLOT_COUNT do slots[logical] = actionState(logical) end
    return { slots = slots }
end

function Bridge:PublishSnapshot()
    if not WCS.Native:IsBridgeAvailable() then return false end
    local ok = WCS.Native:PublishBridgeSnapshot(encode({ player = playerState(), actions = allActions() }))
    if ok then
        self.lastActions = {}; for logical = 1, WCS.Constants.SECOND_SCREEN_SLOT_COUNT do self.lastActions[logical] = encode(actionState(logical)) end
    end
    return ok
end

function Bridge:Publish(eventType, data)
    if WCS.Native:IsBridgeAvailable() then WCS.Native:PublishBridgeEvent(eventType, encode(data)) end
end

function Bridge:PublishPlayer() local state = playerState(); self:Publish("player.state", { name = state.name, level = state.level }) end
function Bridge:PublishMoney() self:Publish("player.money", { copper = GetMoney() or 0 }) end
function Bridge:PublishExperience() self:Publish("player.experience", playerState().experience) end
function Bridge:PublishBags() self:Publish("player.bags", playerState().bags) end

function Bridge:ReconcileActions()
    if not WCS.Native:IsBridgeAvailable() then return end
    self.lastActions = self.lastActions or {}
    for logical = 1, WCS.Constants.SECOND_SCREEN_SLOT_COUNT do
        local state = actionState(logical); local serialized = encode(state)
        if self.lastActions[logical] ~= serialized then self.lastActions[logical] = serialized; WCS.Native:PublishBridgeEvent("action.updated", serialized) end
    end
end

function Bridge:GetStatus()
    local raw = WCS.Native:GetBridgeStatus()
    if not raw then return { available = false } end
    local function boolean(name) return raw:match('"' .. name .. '":true') ~= nil end
    local function number(name) return tonumber(raw:match('"' .. name .. '":(%d+)')) end
    local function stringValue(name)
        local marker = '"' .. name .. '":"'; local first = raw:find(marker, 1, true)
        if not first then return "" end
        local index, parts = first + #marker, {}
        local escapes = { ['\\'] = '\\', ['"'] = '"', ['/'] = '/', b = '\b', f = '\f', n = '\n', r = '\r', t = '\t' }
        while index <= #raw do
            local char = raw:sub(index, index)
            if char == '"' then return table.concat(parts) end
            if char == '\\' and index < #raw then index = index + 1; char = escapes[raw:sub(index, index)] or raw:sub(index, index) end
            table.insert(parts, char); index = index + 1
        end
        return ""
    end
    return { available = true, enabled = boolean("enabled"), listening = boolean("listening"), connected = boolean("connected"), paired = boolean("paired"),
        bindAddress = stringValue("bindAddress"), port = number("port") or 0, device = stringValue("device"), pairingCode = stringValue("pairingCode") }
end

function Bridge:IsAvailable() return WCS.Native:IsBridgeAvailable() end
function Bridge:GetHost() local state = self:GetStatus(); return state.bindAddress or "" end
function Bridge:GetPort() local state = self:GetStatus(); return state.port and state.port > 0 and state.port or WCS.Constants.DEFAULT_PORT end
function Bridge:GetPairingCode() local state = self:GetStatus(); return state.pairingCode or "" end
function Bridge:GetConnectionState()
    local state = self:GetStatus()
    if not state.available then return "unavailable" end
    if state.connected then return "connected" end
    if state.listening then return state.paired and "waiting" or "pairing-required" end
    return "disconnected"
end
function Bridge:RegeneratePairingCode() return WCS.Native:RegeneratePairingCode() end

function Bridge:Initialize()
    if self.initialized then return end; self.initialized = true
    self.wasAvailable = WCS.Native:IsBridgeAvailable(); if self.wasAvailable then self:PublishSnapshot() end
end

function Bridge:OnEvent(event)
    if event == "PLAYER_ENTERING_WORLD" then self:PublishSnapshot()
    elseif event == "PLAYER_LEVEL_UP" then self:PublishPlayer(); self:PublishExperience()
    elseif event == "PLAYER_XP_UPDATE" or event == "UPDATE_EXHAUSTION" then self:PublishExperience()
    elseif event == "PLAYER_MONEY" then self:PublishMoney()
    elseif event == "BAG_UPDATE" then self:PublishBags()
    elseif event == "ACTIONBAR_SLOT_CHANGED" or event == "ACTIONBAR_UPDATE_COOLDOWN" or event == "ACTIONBAR_UPDATE_USABLE" or event == "ACTIONBAR_UPDATE_STATE" then self:ReconcileActions() end
end

function Bridge:Tick(elapsed)
    self.elapsed = (self.elapsed or 0) + elapsed; self.statusElapsed = (self.statusElapsed or 0) + elapsed
    if self.elapsed >= 1 then
        self.elapsed = 0
        local available = WCS.Native:IsBridgeAvailable()
        if available and (not self.wasAvailable or self:GetStatus().connected) then self:PublishSnapshot()
        elseif available then self:ReconcileActions() end
        self.wasAvailable = available
    end
    if self.statusElapsed >= 2 then self.statusElapsed = 0; if WCS.Settings and WCS.Settings.RefreshBridge then WCS.Settings:RefreshBridge() end end
end

Bridge.JsonNull = Null
