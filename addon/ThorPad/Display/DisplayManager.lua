local addonName, ThorPad = ...

ThorPad.Display = { pending = false, barState = {} }
local Display = ThorPad.Display
local reservedBars = { "MultiBarRight", "MultiBarLeft" }

function Display:Initialize()
    if self.initialized then return end; self.initialized = true
    if hooksecurefunc and UIParent_ManageFramePositions then hooksecurefunc("UIParent_ManageFramePositions", function() Display:Reconcile() end) end
end

function Display:GetMode()
    local controller, screen = ThorPadDB.controller.enabled, ThorPadDB.secondScreen.enabled
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
function Display:Apply()
    if InCombatLockdown() then self.pending = true; return end
    self.pending = false; local controller, screen = ThorPadDB.controller.enabled, ThorPadDB.secondScreen.enabled
    if ThorPad.ActionOverlay and ThorPad.ActionOverlay.frame then if controller then ThorPad.ActionOverlay.frame:Show() else ThorPad.ActionOverlay.frame:Hide() end end
    self:SetReservedBarsHidden(screen); ThorPad.UIReduction:Apply(controller and screen and ThorPadDB.secondScreen.reduceUI)
end
function Display:Reconcile()
    if InCombatLockdown() then return end
    if self.pending and not InCombatLockdown() then self:Apply(); return end
    if ThorPadDB.secondScreen.enabled then self:SetReservedBarsHidden(true) end; ThorPad.UIReduction:Reconcile()
end
