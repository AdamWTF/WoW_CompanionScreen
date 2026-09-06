local addonName, WCS = ...

WCS.Display = { pending = false, barState = {} }
local Display = WCS.Display
local reservedBars = { "MultiBarRight", "MultiBarLeft" }

function Display:Initialize()
    if self.initialized then return end; self.initialized = true
    if hooksecurefunc and UIParent_ManageFramePositions then hooksecurefunc("UIParent_ManageFramePositions", function() Display:Reconcile() end) end
end

function Display:GetMode()
    local controller, screen = WCSDB.controller.enabled, WCSDB.secondScreen.enabled
    if controller and screen then return "controller-second-screen" elseif controller then return "controller" elseif screen then return "second-screen" end
    return "standard"
end
function Display:SetReservedBarsHidden(hidden)
    for _, name in ipairs(reservedBars) do
        local frame = _G[name]
        if frame then
            if hidden and not self.barState[name] then self.barState[name] = { alpha = frame:GetAlpha(), mouse = frame:IsMouseEnabled(), shown = frame:IsShown() } end
            local state = self.barState[name]
            if hidden then frame:SetAlpha(0); frame:EnableMouse(false); frame:Hide()
            elseif state then frame:SetAlpha(state.alpha or 1); frame:EnableMouse(state.mouse and true or false); if state.shown then frame:Show() else frame:Hide() end; self.barState[name] = nil end
        end
    end
end
function Display:ShouldHidePartyFrames(reduced)
    if not reduced or not WCS.Bridge then return false end
    local bridge = WCS.Bridge:GetStatus()
    if not bridge.connected or not bridge.partyCapable then return false end
    if GetNumRaidMembers and GetNumRaidMembers() > 0 then return false end
    return GetNumPartyMembers and GetNumPartyMembers() > 0
end
function Display:Apply()
    if InCombatLockdown() then self.pending = true; return end
    self.pending = false; local controller, screen = WCSDB.controller.enabled, WCSDB.secondScreen.enabled
    if WCS.ActionOverlay and WCS.ActionOverlay.frame then if controller then WCS.ActionOverlay.frame:Show() else WCS.ActionOverlay.frame:Hide() end end
    local reduced = controller and screen and WCSDB.secondScreen.reduceUI
    self:SetReservedBarsHidden(screen); WCS.UIReduction:Apply(reduced, self:ShouldHidePartyFrames(reduced))
end
function Display:Reconcile()
    if InCombatLockdown() then return end
    if self.pending and not InCombatLockdown() then self:Apply(); return end
    if WCSDB.secondScreen.enabled then self:SetReservedBarsHidden(true) end
    local reduced = WCSDB.controller.enabled and WCSDB.secondScreen.enabled and WCSDB.secondScreen.reduceUI
    WCS.UIReduction:Apply(reduced, self:ShouldHidePartyFrames(reduced)); WCS.UIReduction:Reconcile()
end
