local addonName, WCS = ...

WCS.SecondScreen = {}

function WCS.SecondScreen:GetActionID(slot)
    slot = tonumber(slot)
    if not slot or slot < 1 or slot > WCS.Constants.SECOND_SCREEN_SLOT_COUNT or slot ~= math.floor(slot) then return nil end
    return WCS.Constants.SECOND_SCREEN_FIRST_ACTION + slot - 1
end

function WCS.SecondScreen:GetActionInfo(slot)
    local actionID = self:GetActionID(slot)
    if not actionID then return nil end
    return GetActionInfo(actionID)
end

function WCS.SecondScreen:IsEnabled() return WCSDB.secondScreen.enabled end
