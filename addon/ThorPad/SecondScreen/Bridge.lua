local addonName, ThorPad = ...

ThorPad.Native = {}

function ThorPad.Native:GetCurrentLayer()
    local value = WXLGamepadNativeLayer
    if type(value) == "string" then value = string.lower(value):gsub("%s", ""):gsub("%+", "") end
    if value == "default" or value == "l2" or value == "r2" or value == "l2r2" then return value end
    return ({ [1] = "default", [2] = "l2", [3] = "r2", [4] = "l2r2" })[tonumber(value)] or "default"
end

function ThorPad.Native:IsControllerAvailable() return WXLGamepadNativeLayer ~= nil or type(WXLGamepadIsAvailable) == "function" end
function ThorPad.Native:IsBridgeAvailable() return type(WXLThorBridgeGetStatus) == "function" end
function ThorPad.Native:PublishBridgeSnapshot(json) return type(WXLThorBridgePublishSnapshot) == "function" and WXLThorBridgePublishSnapshot(json) or false end
function ThorPad.Native:PublishBridgeEvent(kind, json) return type(WXLThorBridgePublishEvent) == "function" and WXLThorBridgePublishEvent(kind, json) or false end
function ThorPad.Native:GetBridgeStatus() return type(WXLThorBridgeGetStatus) == "function" and WXLThorBridgeGetStatus() or nil end
function ThorPad.Native:RegeneratePairingCode()
    if type(WXLThorBridgeRegeneratePairingCode) == "function" then return WXLThorBridgeRegeneratePairingCode() end
    if type(WXLThorBridgeForgetDevice) == "function" then return WXLThorBridgeForgetDevice() end
    return false
end

