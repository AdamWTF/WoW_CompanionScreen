# WoW Companion Screen add-on

WoW Companion Screen targets World of Warcraft 3.3.5a build 12340. Open its native-style configuration window with the minimap button, `/wcs`, or `/wowcompanionscreen`.

## Native controller integration contract

Controller mappings mirror Blizzard's action bars and are exposed through globally named `SecureActionButtonTemplate` frames. The Default layer follows the live primary page/form action slots; modifier layers remain fixed. Controls use positional names (`DPadUp`, `DPadDown`, `DPadLeft`, `DPadRight`, `South`, `East`, `West`, and `North`) through `WCS.Controller:GetButton(layer, control)`.

The deterministic global button names are:

- `WCSControllerDefault1` through `WCSControllerDefault8`
- `WCSControllerL2_1` through `WCSControllerL2_8`
- `WCSControllerR2_1` through `WCSControllerR2_8`
- `WCSControllerL2R2_1` through `WCSControllerL2R2_8`

Within every layer the numeric order is `DPadUp`, `DPadDown`, `DPadLeft`, `DPadRight`, `Y`, `X`, `B`, `A`. Native integration should use the semantic API rather than depending on these indices.

The native action mapping is Default `1-8`, L2 `9-12,49-52`, R2 `53-60`, and L2+R2 `61-68`. These are the MainMenuBar, MultiBarBottomLeft, and MultiBarBottomRight action ranges; WoW itself persists their contents.

WCS System Actions are SavedVariables-backed overrides for these logical positions. The ordered palette contains `JUMP` followed by `INTERACT`. Assigning one clears the native WoW action slot; the gamepad extension dispatches native Jump begin/end behavior or one Smart Interact attempt on press from the logical controller position. Unknown System Action IDs remain persisted but are inert.

The controller extension is expected to publish the active semantic layer as `WCSGamepadNativeLayer`: `default`, `l2`, `r2`, or `l2r2` (numeric values 1-4 are also accepted for compatibility).

The addon synchronizes overrides through `WCSGamepadResetSystemActions`, `WCSGamepadSetSystemAction(layer, control, id)`, and `WCSGamepadSupportsSystemAction(id)` when those globals are available.

## Controller UI navigation

UI navigation is enabled by default with controller support and can be disabled independently in WCS settings. Out of combat, supported stock panels redirect D-pad input to spatial focus, South to confirm, and East to back while suppressing gameplay controls. The confirm/back orientation can be reversed without changing combat mappings. The first supported set is the game menu, static confirmation popups, gossip and quest dialogs, merchants, bags, and WCS's own settings. Bag confirmation uses the stock right-click use/equip behavior.

The native extension exposes `WCSGamepadSetUINavigationActive(active)`, `WCSGamepadMovePointer(normalizedX, normalizedY)`, and `WCSGamepadClickPointer(button)`. WCS supplies `WCS.UINavigation:Handle(command)` for the native extension to dispatch `up`, `down`, `left`, `right`, `confirm`, and `back`. Navigation deactivates during combat and whenever no supported panel is visible.

## Second-screen contract

WCS slots 1-24 directly represent WoW action IDs 25-48. They are never duplicated into `WCSDB`; WoW remains authoritative. `WCS.SecondScreen:GetActionID(slot)` is the public mapping API.
