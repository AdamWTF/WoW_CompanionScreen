local WCS = { Bridge = { PublishParty = function() end } }
local units = { player = "self", party1 = "first", party2 = "second" }
local raid, count, leader, lfg, serial = 0, 2, true, false, 0
local calls, queued, result = {}, nil, nil
GetNumRaidMembers = function() return raid end
GetNumPartyMembers = function() return count end
UnitGUID = function(unit) return units[unit] end
UnitIsPartyLeader = function(unit) return unit == "player" and leader end
UnitName = function(unit) return units[unit], "Realm" end
HasLFGRestrictions = function() return lfg end
WCSBridgePartyGeneration = function() serial = serial + 1; return "generation-" .. serial end
WCSBridgeTakePartyCommand = function() if not queued then return end; local command = queued; queued = nil; return unpack(command) end
WCSBridgePartyResult = function(_, status) result = status end
UninviteUnit = function(name) calls[#calls + 1] = { "remove", name } end
PromoteToLeader = function(unit) calls[#calls + 1] = { "promote", unit } end
LeaveParty = function() calls[#calls + 1] = { "leave" } end
assert(loadfile(arg[1] or "addon/WoWCompanionScreen/PartyManagement.lua"))("WCS", WCS)
local management = WCS.PartyManagement
UninviteUnit = nil
assert(management:State().canRemove, "native removal availability must not depend on protected Lua wrapper")
UninviteUnit = function(name) calls[#calls + 1] = { "remove", name } end
local function run(operation, generation, guid)
    queued = { operation, 1, generation or management:State().generation, "request", guid or units.party1 }
    management:Tick()
    return result
end
local generation = management:State().generation
assert(management:State().generation == generation, "stable reads must not change generation")
assert(run("remove") == "dispatched" and calls[1][2] == "first-Realm", "resolve full authoritative name without a world object")
assert(run("promote") == "dispatched" and calls[2][2] == "party1")
units.party1, units.party2 = units.party2, units.party1
assert(run("remove", generation, "first") == "stale-party", "reorder rejects old confirmation")
generation = management:State().generation
units.party1 = nil; management:State(); units.party1 = "second"
assert(run("remove", generation) == "stale-party", "departure and rejoin cannot reuse generation")
generation = management:State().generation; leader = false
assert(run("promote", generation) == "stale-party", "leadership change invalidates token")
assert(run("remove") == "not-permitted")
assert(run("leave") == "dispatched", "nonleaders can leave")
leader = true; lfg = true
assert(run("remove") == "not-permitted" and run("promote") == "not-permitted")
assert(run("leave") == "dispatched", "LFD allows leave only")
raid = 5
assert(run("leave") == "not-permitted")
raid = 0; lfg = false
assert(run("remove", nil, "replacement") == "stale-party", "final GUID guard")
generation = management:State().generation; management:Invalidate()
assert(run("remove", generation) == "stale-party", "world transition invalidates generation")
UninviteUnit = function() error("client restriction") end
assert(run("remove") == "client-rejected", "stock restrictions retained")
print("Party management Lua regression tests passed")
