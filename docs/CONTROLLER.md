# Native controller support

Controller input runs inside the 32-bit WoW 3.3.5a client.

## Default layout

| Control | Action |
|---|---|
| Left stick | Move and strafe |
| Right stick | Camera |
| L1 / R1 | Previous / next hostile target |
| L3 / R3 | Cycle camera view / next friendly target |
| Start / Select | Game menu / world map |
| L2 / R2 | Select Default, L2, R2, or L2+R2 layer |
| D-pad + face buttons | Eight actions in the active layer |

Face buttons use positional names: South (A/Cross), East (B/Circle), West (X/Square), and North (Y/Triangle).

| Layer | WoW action IDs |
|---|---|
| Default | Live ActionButton 1–8 slots for the current page/form |
| L2 | 9–12, 49–52 |
| R2 | 53–60 |
| L2+R2 | 61–68 |

Each layer orders D-pad Up, Down, Left, Right, then South, East, West, North.

## Mapping actions

Open **Controller Mapping** with `/wcs`, select a layer, and drag spells, items, macros, or existing actions into its slots. Right-click to clear. Protected mappings can only change out of combat.

The **System Actions** palette provides:

- **Jump:** uses WoW's native jump control, including swimming and flying ascent.
- **Interact:** preserves valid targets, including lootable and skinnable corpses; avoids automatic retargeting in combat; and otherwise selects a nearby NPC or game object using distance and camera direction.

Smart Interact defaults to 12 yards and a 60-degree horizontal half-cone. Set `SmartInteractDebug=1` in `wcs-gamepad.cfg` to log candidate scoring.

## UI navigation

When enabled, out of combat, and a supported panel is open:

- D-pad moves focus.
- South confirms and East goes back by default.
- **Menu Confirm** under **Display & Connection** reverses those menu controls without changing combat assignments.
- Gameplay controls pause until the panel closes or combat starts.

Supported panels include the game menu, confirmation popups, gossip, quests, merchants, bags, and WCS settings. Bag confirmation uses WoW's normal right-click action.

## Backends

- **XInput:** Xbox-compatible controllers; no extra runtime.
- **SDL3 Gamepad:** mapped PlayStation and handheld controllers; requires a 32-bit SDL3 runtime beside `Wow.exe`.
- **SDL3 Joystick:** diagnostics for devices without a verified `gamecontrollerdb.txt` mapping.

`Backend=Auto` selects an available backend. Glyphs can be automatic or forced to Xbox, PlayStation, or Thor. Supported DualShock touchpads provide cursor, left-click, and two-finger right-click.

For advanced settings, copy `Extensions\wcs-gamepad\wcs-gamepad.cfg.example` to `wcs-gamepad.cfg`. It controls deadzones, camera response, inversion, trigger thresholds, polling, targeting keys, and diagnostics.

Controller discovery works before entering the world, but gameplay input does not. Disconnects, focus loss, loading, backend changes, and UI-mode changes release held inputs and require neutral controls before resuming.
