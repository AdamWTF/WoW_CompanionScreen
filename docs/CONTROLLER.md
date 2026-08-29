# Native controller support

WoW Companion Screen adds controller input directly to the 32-bit World of Warcraft 3.3.5a client. It does not translate a controller into keyboard input through a separate desktop application: movement, camera control, action activation, Jump, Smart Interact, and supported UI navigation are dispatched inside the game process.

## Default controls

| Control | Gameplay behavior |
|---|---|
| Left stick | Move forward/backward and strafe, including diagonals |
| Right stick | Turn and pitch the camera using relative mouse movement |
| L1 / left shoulder | Previous hostile target |
| R1 / right shoulder | Next hostile target |
| L3 / left-stick click | Cycle through WoW camera views |
| R3 / right-stick click | Next friendly target |
| Start / Menu / Options | Toggle the WoW game menu |
| Select / View / Share | Toggle all bags |
| L2 and R2 | Select the L2, R2, or L2+R2 action layer |
| D-pad and four face buttons | Activate one of eight actions in the selected layer |

Face buttons are described by position so the layout remains consistent between devices: South is A/Cross, East is B/Circle, West is X/Square, and North is Y/Triangle.

The 32 action positions use WoW's own action slots, so their contents are persisted by the game:

| Layer | WoW action slots, ordered D-pad Up/Down/Left/Right then South/East/West/North |
|---|---|
| Default | 1–8 |
| L2 | 9–12 and 49–52 |
| R2 | 53–60 |
| L2 + R2 | 61–68 |

The current layer and its assignments are shown by the in-game action overlay. Trigger press/release hysteresis prevents accidental layer changes around the trigger threshold.

## Remapping and system actions

Open the WoW Companion Screen settings with the minimap button, `/wcs`, or `/wowcompanionscreen`. On **Controller Mapping**, select a layer and drag a spell, item, macro, or WoW action onto any of its eight positions. Right-click a position to clear it. Protected assignments can only be changed out of combat.

Two native **System Actions** can also be dragged onto any position:

- **Jump** uses WoW's native jump control, including swimming or flying ascent. It does not synthesize the Space key.
- **Interact** performs one conservative Smart Interact attempt. It preserves an already identified interactable target, avoids automatically replacing targets in combat, and otherwise selects an eligible NPC or game object using distance, camera alignment, and interaction metadata.

Smart Interact searches within 12 yards and a 60-degree horizontal half-cone by default. Enable `SmartInteractDebug=1` in `Extensions/wcs-gamepad/wcs-gamepad.cfg` when diagnosing candidate selection.

## Controller UI navigation

Controller UI navigation is enabled independently in **Display & Connection**. Outside combat, opening a supported panel changes the controls temporarily:

- D-pad moves spatial focus.
- South confirms or activates the highlighted item.
- East goes back.
- Gameplay movement, camera, targeting, and action input are suppressed until the panel closes or combat begins.

The supported set includes the game menu, confirmation popups, gossip and quest dialogs, merchants, bags, and WoW Companion Screen's own settings. Bag-item confirmation uses WoW's normal right-click use/equip behavior.

## Devices and backends

The default `Backend=Auto` supports:

- XInput controllers without an additional runtime.
- SDL3 Gamepad devices, including mapped PlayStation and handheld controllers, when a Win32 SDL3 runtime is placed beside `Wow.exe`.
- A diagnostic SDL joystick mode for gathering information about unmapped devices before adding a verified mapping to `gamecontrollerdb.txt`.

Glyphs can be selected automatically or forced to Xbox, PlayStation, or AYN Thor style. On SDL-supported DualShock controllers, the touchpad provides relative cursor movement, normal click, and two-finger right-click.

Copy `Extensions/wcs-gamepad/wcs-gamepad.cfg.example` to `wcs-gamepad.cfg` to tune deadzones, camera sensitivity and response, Y inversion, trigger thresholds, polling rate, and targeting key chords. Press **F9** for the native diagnostic overlay; use `/wcs` for player-facing mappings and display settings.

## Runtime safety

Controller polling occurs on a worker, but game input is applied only from WoW's main update event while the character is in the rendered world. Login, realm, character-selection, and loading screens are diagnostic-only. Disconnects, backend changes, focus loss, world transitions, and UI/gameplay mode changes release input owned by the extension and require neutral controls before input resumes.
