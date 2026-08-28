local addonName, ThorPad = ...

ThorPad.Native = {}

function ThorPad.Native:GetCurrentLayer()
    local value = WXLGamepadNativeLayer
    if type(value) == "string" then value = string.lower(value):gsub("%s", ""):gsub("%+", "") end
    if value == "default" or value == "l2" or value == "r2" or value == "l2r2" then return value end
    return ({ [1] = "default", [2] = "l2", [3] = "r2", [4] = "l2r2" })[tonumber(value)] or "default"
end

function ThorPad.Native:IsControllerAvailable() return WXLGamepadNativeLayer ~= nil or type(WXLGamepadIsAvailable) == "function" end
function ThorPad.Native:IsSystemActionBridgeAvailable()
    return type(WXLGamepadResetSystemActions) == "function" and type(WXLGamepadSetSystemAction) == "function" and type(WXLGamepadSupportsSystemAction) == "function"
end
function ThorPad.Native:ResetSystemActions() return type(WXLGamepadResetSystemActions) == "function" and WXLGamepadResetSystemActions() or nil end
function ThorPad.Native:SetSystemAction(layer, control, action) return type(WXLGamepadSetSystemAction) == "function" and WXLGamepadSetSystemAction(layer, control, action) or nil end
function ThorPad.Native:SupportsSystemAction(action) return type(WXLGamepadSupportsSystemAction) == "function" and WXLGamepadSupportsSystemAction(action) or false end
function ThorPad.Native:IsUINavigationAvailable()
    return type(WXLGamepadSetUINavigationActive) == "function" and type(WXLGamepadMovePointer) == "function" and type(WXLGamepadClickPointer) == "function"
end
function ThorPad.Native:SetUINavigationActive(active) return type(WXLGamepadSetUINavigationActive) == "function" and WXLGamepadSetUINavigationActive(active and 1 or 0) or false end
function ThorPad.Native:MoveUINavigationPointer(x, y) return type(WXLGamepadMovePointer) == "function" and WXLGamepadMovePointer(x, y) or false end
function ThorPad.Native:ClickUINavigationPointer(button) return type(WXLGamepadClickPointer) == "function" and WXLGamepadClickPointer(button) or false end
function ThorPad.Native:IsBridgeAvailable() return type(WXLThorBridgeGetStatus) == "function" end
function ThorPad.Native:PublishBridgeSnapshot(json) return type(WXLThorBridgePublishSnapshot) == "function" and WXLThorBridgePublishSnapshot(json) or false end
function ThorPad.Native:PublishBridgeEvent(kind, json) return type(WXLThorBridgePublishEvent) == "function" and WXLThorBridgePublishEvent(kind, json) or false end
function ThorPad.Native:GetBridgeStatus() return type(WXLThorBridgeGetStatus) == "function" and WXLThorBridgeGetStatus() or nil end
function ThorPad.Native:RegeneratePairingCode()
    if type(WXLThorBridgeRegeneratePairingCode) == "function" then return WXLThorBridgeRegeneratePairingCode() end
    if type(WXLThorBridgeForgetDevice) == "function" then return WXLThorBridgeForgetDevice() end
    return false
end
