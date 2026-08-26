local addonName, ThorPad = ...

ThorPad.SecondScreen = {}

function ThorPad.SecondScreen:GetActionID(slot)
    slot = tonumber(slot)
    if not slot or slot < 1 or slot > ThorPad.Constants.SECOND_SCREEN_SLOT_COUNT or slot ~= math.floor(slot) then return nil end
    return ThorPad.Constants.SECOND_SCREEN_FIRST_ACTION + slot - 1
end

function ThorPad.SecondScreen:GetActionInfo(slot)
    local actionID = self:GetActionID(slot)
    if not actionID then return nil end
    return GetActionInfo(actionID)
end

function ThorPad.SecondScreen:IsEnabled() return ThorPadDB.secondScreen.enabled end

