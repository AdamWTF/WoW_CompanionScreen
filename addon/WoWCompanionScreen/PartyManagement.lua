local _, WCS = ...
local Management = { signature = nil, generation = nil }
WCS.PartyManagement = Management

function Management:Invalidate()
    self.signature = nil
    self.generation = nil
end

function Management:State()
    local group = "solo"
    if GetNumRaidMembers and GetNumRaidMembers() > 0 then group = "raid"
    elseif GetNumPartyMembers and GetNumPartyMembers() > 0 then
        local mode = GetLFGMode and GetLFGMode()
        local lfg = (HasLFGRestrictions and HasLFGRestrictions()) or (IsInLFGDungeon and IsInLFGDungeon()) or mode == "lfgparty" or mode == "abandonedInDungeon"
        group = lfg and "dungeon-finder" or "party"
    end
    local leader = UnitIsPartyLeader and UnitIsPartyLeader("player") and true or false
    local identity = { group, tostring(UnitGUID("player")), tostring(leader) }
    for slot = 1, 4 do
        local unit = "party" .. slot
        identity[#identity + 1] = tostring(UnitGUID(unit))
        identity[#identity + 1] = tostring(UnitIsPartyLeader and UnitIsPartyLeader(unit) and true or false)
    end
    local signature = table.concat(identity, ":")
    if signature ~= self.signature or not self.generation then
        self.signature = signature
        self.generation = WCSBridgePartyGeneration and WCSBridgePartyGeneration() or nil
    end
    local available = self.generation ~= nil and type(WCSBridgeTakePartyCommand) == "function"
    return { groupType = group, generation = self.generation,
        canRemove = available and group == "party" and leader and type(UninviteUnit) == "function",
        canPromote = available and group == "party" and leader and type(PromoteToLeader) == "function",
        canLeave = available and (group == "party" or group == "dungeon-finder") and type(LeaveParty) == "function" }
end

-- Called by the add-on's OnUpdate on the game thread. Use ordinary stock API calls:
-- no script execution helper, secure-state spoofing, or protected-action bypass.
function Management:Tick()
    if not WCSBridgeTakePartyCommand or not WCSBridgePartyResult then return end
    local operation, slot, generation, request, expectedGuid = WCSBridgeTakePartyCommand()
    if not operation then return end
    local current = self:State()
    local result = "not-permitted"
    if generation ~= current.generation then result = "stale-party"
    elseif operation == "leave" and current.canLeave then
        result = pcall(LeaveParty) and "dispatched" or "client-rejected"
    elseif (operation == "remove" and current.canRemove) or (operation == "promote" and current.canPromote) then
        local unit = type(slot) == "number" and slot >= 1 and slot <= 4 and slot == math.floor(slot) and ("party" .. slot)
        if not unit or not expectedGuid or UnitGUID(unit) ~= expectedGuid then result = "stale-party"
        else
            local name, realm = UnitName(unit)
            if not name or name == "" then result = "member-unavailable"
            elseif operation == "remove" then
                if realm and realm ~= "" then name = name .. "-" .. realm end
                result = pcall(UninviteUnit, name) and "dispatched" or "client-rejected"
            else result = pcall(PromoteToLeader, unit, 1) and "dispatched" or "client-rejected" end
        end
    end
    WCSBridgePartyResult(request, result)
    WCS.Bridge:PublishParty()
end
