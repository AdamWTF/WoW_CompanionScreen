# wcs-gamepad

`wcs-gamepad` provides in-process controller support for the 32-bit WoW 3.3.5a build 12340 client. Hardware is normalized behind XInput, SDL3 Gamepad, or diagnostic SDL3 Joystick backends before any gameplay behavior runs.

## Runtime layout

- Left stick: forward/backward and strafe left/right, including diagonals.
- Right stick: relative mouse movement while WCS owns a synthetic right mouse hold.
- L1/R1: previous/next hostile target.
- L3: cycle through WoW's camera views. R3: next friendly target.
- Start (Xbox Menu/Start, PlayStation Options, Thor Start): toggle the WoW game menu.
- Select (Xbox View/Back, PlayStation Share/Create, Thor Select): toggle all bags.
- L2/R2: Base, L2, R2, and L2+R2 action layers with trigger hysteresis.
- D-pad plus South/East/West/North: 32 logical WCS actions, defaulting to the existing action-bar assignments.
- DualShock touchpad: relative cursor, one-finger/physical click, and two-finger right-click when supplied by SDL.
- Supported WCS UI panels: D-pad spatial navigation, South confirm, and East back. Gameplay movement, camera, targeting, and actions are suppressed until the panel closes or combat begins.

Controller polling runs at 125 Hz on a worker. WoW movement, action, keyboard, mouse, UI, and camera calls are applied only from the main `OnUpdate` event while the world-render lifecycle is active. Login, realm, character-selection, and loading screens remain diagnostic-only. Disconnects, backend changes, focus loss, world leave, and UI/gameplay mode transitions release every input owned by the extension and wait for neutral input before resuming. Pointer requests made by Lua are queued until its current update has returned, preventing reentrant UI-script execution.

The WCS addon may override any logical action through the extension's in-process Lua API. `JUMP` uses WoW's native Jump input-control bit on press and release, including swimming/flying ascent; it never synthesizes Space. `INTERACT` runs Smart Interact once per press: it preserves positively identified interactable targets, suppresses automatic replacement in combat, and otherwise scores conservative NPC/GameObject candidates by camera alignment, distance, and verified interaction metadata. Unsupported System Action IDs are retained as inert mappings and logged rather than falling through to a WoW action.

Smart Interact defaults to a 12-yard search radius and a 60-degree horizontal half-cone. `SmartInteractDebug=1` adds native diagnostic logging for eligible candidates, the selected candidate, score components, and the result; it never writes addon chat output.

## Installation and configuration

Place the Win32 `wcs-gamepad.dll` in `Extensions/wcs-gamepad/`. SDL support additionally needs a Win32 SDL3 runtime beside `Wow.exe`; XInput does not. Keep `gamecontrollerdb.txt` beside the extension DLL.

Copy `wcs-gamepad.cfg.example` to `wcs-gamepad.cfg`. The default `Backend=Auto` order is XInput, SDL Gamepad, SDL Joystick under Wine/GameNative and SDL Gamepad, XInput, SDL Joystick on native Windows. Explicit backend choices do not cross-fallback. Every section key also has a flat/environment alias such as `WCS_GAMEPAD_BACKEND`.

An unmapped SDL joystick is intentionally diagnostic-only. Set `ControllerDebug=1`, record its GUID plus button/axis/hat transitions, and add a verified SDL mapping entry to `gamecontrollerdb.txt`; it will then enter through SDL's standardized Gamepad path.

`GlyphStyle=PlayStation`, `Xbox`, or `Thor` forces presentation. With `Auto`, the addon's persisted selector applies, and its Auto option uses the active backend's device hint. Glyph selection never changes input.

## Manual acceptance

For AYN Thor/GameNative, Xbox, and DualShock, first exercise every controller input on the login, realm, character-selection, and loading screens; discovery diagnostics may update, but no gameplay/UI command may run and the client must remain stable. In-world, verify every cardinal/diagonal movement, right-stick camera and RMB release, L1/R1 hostile targeting, R3 friendly targeting, L3 camera-view cycling, Start game-menu toggling, Select all-bag toggling, trigger hysteresis and all four layers, all 32 action cells, disconnect/reconnect, focus loss, and physical mouse/touchscreen coexistence. Confirm held fixed controls do not repeat or fire when entering the world or returning focus. On DualShock also verify touchpad cursor and taps. Finally verify the WCS WebSocket connection, 24 second-screen actions, keyboard, touchpad, shortcuts, game-state export, and persistent settings.
