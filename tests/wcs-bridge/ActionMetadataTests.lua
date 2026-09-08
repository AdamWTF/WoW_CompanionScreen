-- Run with Lua 5.1+: lua ActionMetadataTests.lua addon/WoWCompanionScreen/Bridge.lua
local source = assert(arg[1], "Bridge.lua path required")
local actions, events = {}, {}
local WCS = {
    Constants = { SECOND_SCREEN_SLOT_COUNT = 24 },
    SecondScreen = { GetActionID = function(_, slot) return slot + 24 end },
    Native = {
        IsBridgeAvailable = function() return true end,
        PublishBridgeEvent = function(_, kind, payload) events[#events + 1] = {kind, payload}; return true end,
    },
}
BOOKTYPE_SPELL = "spell"
GetActionInfo = function(slot) local a = actions[slot]; return a.kind, a.id end
HasAction = function(slot) return actions[slot] ~= nil end
GetSpellInfo = function() error("must not interpret a spellbook index as a global spell ID") end
GetSpellName = function(index, book)
    assert(book == BOOKTYPE_SPELL)
    if index == 7 then return "Flash Heal" end -- global spell 7 is unrelated
    if index == 8 then return "Renew" end
end
GetItemInfo = function(id) if id == 6948 then return "Hearthstone" end end
GetMacroInfo = function(id) if id == 1 then return "My healing macro" end end
GetActionTexture = function(slot) return actions[slot].icon or "Interface\\Icons\\Spell_Holy_FlashHeal" end
GetActionText = function(slot) return actions[slot].text end
IsUsableAction = function() return true, false end
IsActionInRange = function() return 1 end
GetActionCooldown = function() return 0, 0, 0 end
GetTime = function() return 0 end
GetActionCount = function() return 0 end
IsCurrentAction = function() return false end
IsEquippedAction = function() return false end
assert(loadfile(source))("WCS", WCS)
local function expectName(slot, kind, id, name, text)
    actions[slot + 24] = {kind = kind, id = id, text = text}
    events = {}
    WCS.Bridge:ReconcileActions()
    local found
    for _, event in ipairs(events) do
        if event[2]:find('"slot":' .. slot .. ',', 1, true) then found = event[2] end
    end
    assert(found and found:find('"name":"' .. name .. '"', 1, true), found or "missing slot update")
end
expectName(1, "spell", 7, "Flash Heal")
expectName(1, "spell", 8, "Renew") -- replacement must not retain the old name
expectName(2, "spell", 999, "")
expectName(3, "spell", 999, "Action text", "Action text")
expectName(4, "item", 6948, "Hearthstone")
expectName(5, "macro", 1, "My healing macro")
expectName(6, "item", 999, "")
expectName(6, "item", 6948, "Hearthstone") -- uncached item subsequently resolves
events = {}; WCS.Bridge:ReconcileActions(); assert(#events == 0, "unchanged actions must be deduplicated")
actions[25] = nil; events = {}; WCS.Bridge:ReconcileActions()
assert(#events == 1 and events[1][2]:find('"empty":true', 1, true))
print("WCS action metadata tests passed")
