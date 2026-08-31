local addonName, WCS = ...

WCS.Native = {}

function WCS.Native:GetCurrentLayer()
    local value = WCSGamepadNativeLayer
    if type(value) == "string" then value = string.lower(value):gsub("%s", ""):gsub("%+", "") end
    if value == "default" or value == "l2" or value == "r2" or value == "l2r2" then return value end
    return ({ [1] = "default", [2] = "l2", [3] = "r2", [4] = "l2r2" })[tonumber(value)] or "default"
end

function WCS.Native:IsControllerAvailable() return WCSGamepadNativeLayer ~= nil or type(WCSGamepadIsAvailable) == "function" end
function WCS.Native:IsSystemActionBridgeAvailable()
    return type(WCSGamepadResetSystemActions) == "function" and type(WCSGamepadSetSystemAction) == "function" and type(WCSGamepadSetWoWAction) == "function" and type(WCSGamepadSupportsSystemAction) == "function" and type(WCSGamepadSetMenuConfirm) == "function"
end
function WCS.Native:ResetSystemActions() return type(WCSGamepadResetSystemActions) == "function" and WCSGamepadResetSystemActions() or nil end
function WCS.Native:SetSystemAction(layer, control, action) return type(WCSGamepadSetSystemAction) == "function" and WCSGamepadSetSystemAction(layer, control, action) or nil end
function WCS.Native:SetWoWAction(layer, control, slot) return type(WCSGamepadSetWoWAction) == "function" and WCSGamepadSetWoWAction(layer, control, slot) or nil end
function WCS.Native:SupportsSystemAction(action) return type(WCSGamepadSupportsSystemAction) == "function" and WCSGamepadSupportsSystemAction(action) or false end
function WCS.Native:IsUINavigationAvailable()
    return type(WCSGamepadSetUINavigationActive) == "function" and type(WCSGamepadMovePointer) == "function" and type(WCSGamepadClickPointer) == "function"
end
function WCS.Native:SetUINavigationActive(active) return type(WCSGamepadSetUINavigationActive) == "function" and WCSGamepadSetUINavigationActive(active and 1 or 0) or false end
function WCS.Native:SetMenuConfirm(control) return type(WCSGamepadSetMenuConfirm) == "function" and WCSGamepadSetMenuConfirm(control) or false end
function WCS.Native:MoveUINavigationPointer(x, y) return type(WCSGamepadMovePointer) == "function" and WCSGamepadMovePointer(x, y) or false end
function WCS.Native:ClickUINavigationPointer(button) return type(WCSGamepadClickPointer) == "function" and WCSGamepadClickPointer(button) or false end
function WCS.Native:IsBridgeAvailable() return type(WCSBridgeGetStatus) == "function" end
function WCS.Native:PublishBridgeSnapshot(json) return type(WCSBridgePublishSnapshot) == "function" and WCSBridgePublishSnapshot(json) or false end
function WCS.Native:PublishBridgeEvent(kind, json) return type(WCSBridgePublishEvent) == "function" and WCSBridgePublishEvent(kind, json) or false end
function WCS.Native:GetBridgeStatus() return type(WCSBridgeGetStatus) == "function" and WCSBridgeGetStatus() or nil end
function WCS.Native:RegeneratePairingCode()
    if type(WCSBridgeRegeneratePairingCode) == "function" then return WCSBridgeRegeneratePairingCode() end
    if type(WCSBridgeForgetDevice) == "function" then return WCSBridgeForgetDevice() end
    return false
end
