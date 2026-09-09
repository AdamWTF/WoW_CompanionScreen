local WCS = {}
local roster, raid, combat = {}, 0, false
local bridge = { connected = true, partyCapable = true }
WCS.Bridge = { GetStatus = function() return bridge end }
WCSDB = { controller = { enabled = true }, secondScreen = { enabled = true, reduceUI = true } }
InCombatLockdown = function() return combat end
GetNumRaidMembers = function() return raid end
GetNumPartyMembers = function() local count = 0; for _ in pairs(roster) do count = count + 1 end; return count end

local function frame(id, shown)
    return { id = id, shown = shown, alpha = .8, mouse = true, mutations = 0,
        IsShown = function(self) return self.shown end,
        GetAlpha = function(self) return self.alpha end,
        IsMouseEnabled = function(self) return self.mouse end,
        SetAlpha = function(self, value) self.alpha = value; self.mutations = self.mutations + 1 end,
        EnableMouse = function(self, value) self.mouse = value end,
        Show = function(self) self.shown = true end,
        Hide = function(self) self.shown = false end }
end
for slot = 1, 4 do _G["PartyMemberFrame" .. slot] = frame(slot, slot == 1) end
MainMenuBar = frame(0, true)
PlayerFrame = frame(0, true)
RaidFrame = frame(0, true)
local updates = 0
-- Model Wrath's UpdateMember visibility decision; the implementation must delegate to it.
PartyMemberFrame_UpdateMember = function(self)
    updates = updates + 1
    assert(WCS.UIReduction.saved["PartyMemberFrame" .. self.id] == nil, "clear saved state before callbacks")
    if (HIDE_PARTY_INTERFACE == "1" and raid > 0) or not roster[self.id] then self:Hide() else self:Show() end
end
assert(loadfile("addon/WoWCompanionScreen/Display/UIReduction.lua"))("WCS", WCS)
assert(loadfile("addon/WoWCompanionScreen/Display/DisplayManager.lua"))("WCS", WCS)

roster = { [1] = true }
WCS.Display:Apply()
assert(not PartyMemberFrame1.shown and PartyMemberFrame1.alpha == 0)
roster = {}
WCS.Display:Reconcile()
for slot = 1, 4 do
    local current = _G["PartyMemberFrame" .. slot]
    assert(not current.shown, "solo must never restore a cached portrait")
    assert(current.alpha == .8 and current.mouse, "restore appearance and input")
end
assert(updates == 4)
WCS.Display:Reconcile()
assert(updates == 4, "restoration only happens once")

roster = { [1] = true }
WCS.Display:Apply()
roster = { [2] = true }; bridge.connected = false
WCS.Display:Reconcile()
assert(not PartyMemberFrame1.shown and PartyMemberFrame2.shown, "disconnect uses current roster, including previously hidden slots")
bridge.connected = true; WCS.Display:Apply()
raid = 5; HIDE_PARTY_INTERFACE = "1"
WCS.Display:Reconcile()
assert(not PartyMemberFrame2.shown, "honor Blizzard raid visibility setting")
raid = 0; WCS.Display:Apply(); raid = 5; HIDE_PARTY_INTERFACE = "0"
WCS.Display:Reconcile()
assert(PartyMemberFrame2.shown, "allow Blizzard party-in-raid presentation when configured")

raid = 0; WCS.Display:Apply()
combat = true; roster = {}
local before = PartyMemberFrame2.mutations
WCS.Display:Reconcile(); WCS.Display:Apply()
assert(PartyMemberFrame2.mutations == before and WCS.Display.pending, "defer mutations during combat")
combat = false; WCS.Display:Reconcile()
assert(not PartyMemberFrame2.shown and not WCS.Display.pending, "combat-safe restore uses current solo state")

roster = { [1] = true }; WCS.Display:Apply()
WCSDB.secondScreen.reduceUI = false; WCS.Display:Apply()
assert(PartyMemberFrame1.shown and MainMenuBar.shown, "feature disable restores stock party and ordinary chrome")
assert(PlayerFrame.shown and RaidFrame.shown and PlayerFrame.mutations == 0 and RaidFrame.mutations == 0, "never mutate player or raid frames")
print("UI reduction Lua regression tests passed")
