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

function ThorPad.Native:IsBridgeAvailable()
    return type(WXLThorBridgePublishSnapshot) == "function"
end

function ThorPad.Native:PublishBridgeSnapshot(json)
    if not self:IsBridgeAvailable() then return false end
    return WXLThorBridgePublishSnapshot(json)
end

function ThorPad.Native:PublishBridgeEvent(eventType, json)
    if type(WXLThorBridgePublishEvent) ~= "function" then return false end
    return WXLThorBridgePublishEvent(eventType, json)
end

function ThorPad.Native:GetBridgeStatus()
    if type(WXLThorBridgeGetStatus) ~= "function" then return nil end
    return WXLThorBridgeGetStatus()
end

function ThorPad.Native:ForgetBridgeDevice()
    if type(WXLThorBridgeForgetDevice) ~= "function" then return false end
    return WXLThorBridgeForgetDevice()
end
