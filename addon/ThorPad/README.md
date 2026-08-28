# ThorPad addon

ThorPad targets World of Warcraft 3.3.5a build 12340. Open its native-style configuration window with the minimap button, `/thorpad`, or `/tp`.

## Native controller integration contract

Controller mappings directly mirror the three horizontal Blizzard action bars and are exposed through globally named `SecureActionButtonTemplate` frames. Controls use positional names (`DPadUp`, `DPadDown`, `DPadLeft`, `DPadRight`, `South`, `East`, `West`, and `North`) through `ThorPad.Controller:GetButton(layer, control)`.

The deterministic global button names are:

- `ThorPadControllerDefault1` through `ThorPadControllerDefault8`
- `ThorPadControllerL2_1` through `ThorPadControllerL2_8`
- `ThorPadControllerR2_1` through `ThorPadControllerR2_8`
- `ThorPadControllerL2R2_1` through `ThorPadControllerL2R2_8`

Within every layer the numeric order is `DPadUp`, `DPadDown`, `DPadLeft`, `DPadRight`, `Y`, `X`, `B`, `A`. Native integration should use the semantic API rather than depending on these indices.

The native action mapping is Default `1-8`, L2 `9-12,49-52`, R2 `53-60`, and L2+R2 `61-68`. These are the MainMenuBar, MultiBarBottomLeft, and MultiBarBottomRight action ranges; WoW itself persists their contents.

ThorPad System Actions are SavedVariables-backed overrides for these logical positions. `JUMP` is the initial registry entry. Assigning it clears the native WoW action slot, while the gamepad extension dispatches native Jump begin/end behavior from the logical controller position. Unknown System Action IDs remain persisted but are inert.

The controller extension is expected to publish the active semantic layer as `WXLGamepadNativeLayer`: `default`, `l2`, `r2`, or `l2r2` (numeric values 1-4 are also accepted for compatibility).

The addon synchronizes overrides through `WXLGamepadResetSystemActions`, `WXLGamepadSetSystemAction(layer, control, id)`, and `WXLGamepadSupportsSystemAction(id)` when those globals are available.

## Second-screen contract

ThorPad slots 1-24 directly represent WoW action IDs 25-48. They are never duplicated into `ThorPadDB`; WoW remains authoritative. `ThorPad.SecondScreen:GetActionID(slot)` is the public mapping API.
