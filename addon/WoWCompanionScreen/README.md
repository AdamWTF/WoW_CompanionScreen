# WoW Companion Screen add-on

Configuration UI for WoW 3.3.5a build 12340. Open it with the minimap button, `/wcs`, or `/wowcompanionscreen`.

## Controller contract

Mappings use globally named `SecureActionButtonTemplate` frames. The Default layer follows the live primary page/form; modifier layers remain fixed. Use `WCS.Controller:GetButton(layer, control)` with `DPadUp`, `DPadDown`, `DPadLeft`, `DPadRight`, `South`, `East`, `West`, or `North`; do not depend on numeric indices.

Secure button globals are:

- `WCSControllerDefault1`–`WCSControllerDefault8`
- `WCSControllerL2_1`–`WCSControllerL2_8`
- `WCSControllerR2_1`–`WCSControllerR2_8`
- `WCSControllerL2R2_1`–`WCSControllerL2R2_8`

The index order is D-pad Up, Down, Left, Right, Y, X, B, A. Modifier action IDs are L2 `9-12,49-52`, R2 `53-60`, and L2+R2 `61-68`.

System Action overrides are stored in SavedVariables. `JUMP` and `INTERACT` clear the corresponding WoW action; unknown IDs remain stored but inert. The add-on synchronizes through `WCSGamepadResetSystemActions`, `WCSGamepadSetSystemAction(layer, control, id)`, and `WCSGamepadSupportsSystemAction(id)`.

The extension publishes `WCSGamepadNativeLayer` as `default`, `l2`, `r2`, or `l2r2`; numeric values 1–4 remain compatible.

## UI navigation contract

The extension exposes `WCSGamepadSetUINavigationActive(active)`, `WCSGamepadMovePointer(normalizedX, normalizedY)`, and `WCSGamepadClickPointer(button)`. It dispatches `up`, `down`, `left`, `right`, `confirm`, and `back` through `WCS.UINavigation:Handle(command)`.

Navigation is out-of-combat only. D-pad moves focus; South confirms and East returns by default. The orientation can be reversed without changing combat mappings. Gameplay input is suppressed while a supported panel is active.

## Second-screen contract

Slots 1–24 map directly to WoW action IDs 25–48 and are not duplicated into `WCSDB`. Use `WCS.SecondScreen:GetActionID(slot)`.
