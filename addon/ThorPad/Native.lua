local addonName, ThorPad = ...

ThorPad.Native = {}

-- This is intentionally the only legacy product identifier in the Lua addon.
-- The native WarcraftXL extension publishes this global and cannot be renamed here.
function ThorPad.Native:GetCurrentLayer()
    local layer = tonumber(WXLGamepadNativeLayer) or 1
    if layer < 1 or layer > 5 then
        return 1
    end
    return layer
end
