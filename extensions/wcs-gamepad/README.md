# wcs-gamepad

In-process controller support for the 32-bit WoW 3.3.5a build 12340 client.

## Runtime

- Left stick moves; right stick controls the camera through an owned right-mouse hold.
- L1/R1 target hostiles; L3 cycles camera views; R3 targets friendlies.
- Start toggles the game menu; Select toggles the world map.
- L2/R2 select four action layers; D-pad and face buttons provide 32 logical actions.
- Supported DualShock touchpads provide cursor and click input.
- Supported panels use D-pad focus with configurable South/East confirm and back while gameplay input is suppressed.

Polling runs at 125 Hz on a worker. WoW calls execute only from the main `OnUpdate` while the world is active. Login, realm, character-selection, and loading screens are diagnostic-only. Disconnects, backend or focus changes, world exit, and UI-mode changes release owned input and require neutral controls before resuming. Lua pointer requests are queued to avoid reentrant UI scripts.

The add-on synchronizes every logical action through the in-process Lua API. The Default layer resolves WoW's live primary action page, including stance and form bonus pages; modifier layers remain fixed. `JUMP` uses WoW's native input bit. `INTERACT` preserves valid targets, recognizes lootable and skinnable corpses, avoids automatic combat retargeting, and otherwise scores conservative NPC/GameObject candidates. Unknown System Action IDs remain inert. Smart Interact defaults to 12 yards and a 60-degree half-cone; `SmartInteractDebug=1` logs scoring.

## Install and configure

Place `wcs-gamepad.dll`, `gamecontrollerdb.txt`, and optional `wcs-gamepad.cfg` in `Extensions/wcs-gamepad/`. Copy the example config to enable overrides.

- XInput needs no extra runtime.
- SDL Gamepad needs a Win32 SDL3 runtime beside `Wow.exe`.
- Unmapped SDL Joystick is diagnostic-only; use `ControllerDebug=1` to collect a mapping.
- `Backend=Auto` prefers XInput, SDL Gamepad, then SDL Joystick under Wine/GameNative; native Windows prefers SDL Gamepad, XInput, then SDL Joystick.
- Explicit backends do not cross-fallback.
- Config keys also support flat/environment aliases such as `WCS_GAMEPAD_BACKEND`.
- `GlyphStyle` accepts `Auto`, `PlayStation`, `Xbox`, or `Thor` and never changes input behavior.

## Manual acceptance

Test AYN Thor/GameNative, Xbox, and DualShock where available:

- Before entering the world, inputs may update diagnostics but must not execute commands or crash.
- In-world, cover movement, camera/RMB release, targeting, camera cycle, game menu, map, trigger hysteresis, all layers/actions, live form/page changes, both menu-confirm orientations, disconnect/reconnect, focus loss, held-input neutralization, and mouse/touch coexistence.
- Test DualShock touchpad input where supported.
- Verify UI navigation and combat suppression.
- Verify WebSocket state, 24 companion actions, keyboard, touchpad, all-bags shortcut, shortcuts, and persistence.
