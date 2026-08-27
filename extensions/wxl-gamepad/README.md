# wxl-gamepad

`wxl-gamepad` provides in-process controller support for the 32-bit WoW 3.3.5a build 12340 client. Hardware is normalized behind XInput, SDL3 Gamepad, or diagnostic SDL3 Joystick backends before any gameplay behavior runs.

## Runtime layout

- Left stick: forward/backward and strafe left/right, including diagonals.
- Right stick: relative mouse movement while ThorPad owns a synthetic right mouse hold.
- L1/R1: previous/next hostile target.
- L3: cycle through WoW's camera views. R3: next friendly target.
- Start (Xbox Menu/Start, PlayStation Options, Thor Start): toggle the WoW game menu.
- Select (Xbox View/Back, PlayStation Share/Create, Thor Select): toggle all bags.
- L2/R2: Base, L2, R2, and L2+R2 action layers with trigger hysteresis.
- D-pad plus South/East/West/North: 32 existing action-bar assignments.
- DualShock touchpad: relative cursor, one-finger/physical click, and two-finger right-click when supplied by SDL.

Controller polling runs at 125 Hz on a worker. WoW movement, action, keyboard, mouse, UI, and camera calls are applied only from the main `OnUpdate` event while the world-render lifecycle is active. Login, realm, character-selection, and loading screens remain diagnostic-only. Disconnects, backend changes, focus loss, and world leave release every input owned by the extension.

## Installation and configuration

Place the Win32 `wxl-gamepad.dll` in `Extensions/wxl-gamepad/`. SDL support additionally needs a Win32 SDL3 runtime beside `Wow.exe`; XInput does not. Keep `gamecontrollerdb.txt` beside the extension DLL.

Copy `wxl-gamepad.cfg.example` to `wxl-gamepad.cfg`. The default `Backend=Auto` order is XInput, SDL Gamepad, SDL Joystick under Wine/GameNative and SDL Gamepad, XInput, SDL Joystick on native Windows. Explicit backend choices do not cross-fallback. Every section key also has a flat/environment alias such as `WXL_GAMEPAD_BACKEND`.

An unmapped SDL joystick is intentionally diagnostic-only. Set `ControllerDebug=1`, record its GUID plus button/axis/hat transitions, and add a verified SDL mapping entry to `gamecontrollerdb.txt`; it will then enter through SDL's standardized Gamepad path.

`GlyphStyle=PlayStation`, `Xbox`, or `Thor` forces presentation. With `Auto`, the addon's persisted selector applies, and its Auto option uses the active backend's device hint. Glyph selection never changes input.

## Manual acceptance

For AYN Thor/GameNative, Xbox, and DualShock, first exercise every controller input on the login, realm, character-selection, and loading screens; discovery diagnostics may update, but no gameplay/UI command may run and the client must remain stable. In-world, verify every cardinal/diagonal movement, right-stick camera and RMB release, L1/R1 hostile targeting, R3 friendly targeting, L3 camera-view cycling, Start game-menu toggling, Select all-bag toggling, trigger hysteresis and all four layers, all 32 action cells, disconnect/reconnect, focus loss, and physical mouse/touchscreen coexistence. Confirm held fixed controls do not repeat or fire when entering the world or returning focus. On DualShock also verify touchpad cursor and taps. Finally verify the ThorPad WebSocket connection, 24 second-screen actions, keyboard, touchpad, shortcuts, game-state export, and persistent settings.
