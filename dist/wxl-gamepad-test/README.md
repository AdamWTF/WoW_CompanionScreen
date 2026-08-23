# wxl-gamepad

`wxl-gamepad` is a proof-of-concept native controller extension for WarcraftXL `v1.1` and the 32-bit World of Warcraft 3.3.5a build 12340 client.  Its input path is entirely in-process:

`DualShock 4 -> SDL3 -> wxl-gamepad -> WarcraftXL OnUpdate -> WoW native input/camera controls`

It does not use DS4Windows, XInput-only input, `SendInput`, injected keyboard/mouse messages, Steam Input, or a companion process. SDL's gamepad mapping recognises a DualShock 4 over either USB or Bluetooth as a gamepad, rather than requiring Xbox emulation.

## Controls

| Control | Client-native action |
| --- | --- |
| Left stick | camera-relative forward/backward + strafe; diagonals work |
| Right stick | camera view left/right/up/down |
| Cross (`SDL_GAMEPAD_BUTTON_SOUTH`) | `JumpOrAscendStart` then `JumpOrAscendStop` on the press edge |

Both sticks use a radial 18% deadzone and actions engage at 35% deflection. This eliminates centred-stick drift and makes release issue the matching native stop action immediately. The present POC converts the post-deadzone analogue values to digital client action state; keeping the analogue values in the input layer makes a later true-analogue engine binding a contained replacement.

The camera uses the client's `MoveView*Start/Stop` controls, whose own per-frame update owns timing; therefore camera motion does not depend on the extension's frame rate. The F9 panel exposes horizontal/vertical inversion toggles, initially enabled for this client. Initial speed is the player's normal WoW keyboard-view speed and can be tuned through the existing in-game camera/keybinding settings.

## Build and install

1. Check out WarcraftXL branch `v1.1`.
2. Copy this `wxl-gamepad` folder to `extensions/wxl-gamepad/` in that checkout (this repository already has it there).
3. Obtain an official SDL3 runtime DLL matching the **Win32/x86** WarcraftXL build and place `SDL3.dll` next to `Wow.exe`. SDL is dynamically loaded: no SDL import library or external process is required.
4. Build Win32, for example: `cmake -S . -B build -A Win32` then `cmake --build build --config Release --target wxl-gamepad`.
5. Place the resulting `wxl-gamepad.dll` at `Wow.exe`'s `Extensions/wxl-gamepad/wxl-gamepad.dll` (the normal `CLIENT_PATH` build option deploys it there).
6. Build/deploy `WarcraftXL.dll` from the same tree and launch only a 3.3.5a build-12340 client.

WarcraftXL validates `WXL_CLIENT_BUILD` during `WXL_Query` before executing `WXL_Load`; a different build is refused rather than guessed.

## Manual test

1. Disable DS4Windows, Steam Input, WoWpadX and every controller mapper.
2. Connect a DualShock 4 by Bluetooth, start the WarcraftXL-enabled client, log in, and inspect the WarcraftXL log.
3. Confirm `SDL3 gamepad subsystem initialised` and `controller connected: Wireless Controller` (or the SDL-provided name).
   Press `F9` to open the WarcraftXL overlay and select the `wxl-gamepad` panel. It displays the
   SDL connection state, deadzoned left/right stick values, and Cross state, so it distinguishes an
   SDL/device problem from a client-action problem without requiring an external controller mapper.
4. Move the left stick in cardinal and diagonal directions, then release it. The character should start the matching movement actions and stop at centre.
5. Move/release the right stick. The camera should continue only while deflected.
6. Press and hold Cross. One jump is issued on the press edge; it is not repeated each frame.
7. Disconnect and reconnect the controller. The log should report disconnection, movement/camera actions stop, and a later connection is picked up automatically.
8. Repeat steps 2-7 over USB if available.

## Engine binding and RE note

The extension never invokes the protected FrameScript `MoveForwardStart`, `JumpOrAscendStart`, or `MoveView*` handlers. Those handlers display WoW's "blocked from an action only available to the Blizzard UI" popup when called from an extension/addon context.

Instead, `wxl::game::input` wraps the lower native `CInputControl` calls found directly beneath the stock handlers: the control pointer is `0x00C24954`, `Begin` is `0x005FA170`, `End` is `0x005FA450`, and `Commit` is `0x005FBBC0`. All are `__thiscall`; the action handlers verify their control IDs as forward `0x10`, backward `0x20`, strafe left `0x40`, strafe right `0x80`, and jump `0x2000`. The extension calls Begin/End only on controller state transitions, then Commit so the client owns movement packets and animation.

The right stick writes the same active-camera view-control flags/timestamps used by the stock `MoveView*` handlers. These fields and indices were verified from the 12340 executable's `0x005FF000` start and `0x005FF120`-`0x005FF230` stop handlers. The client's per-frame camera update consumes them, preserving frame-rate-independent camera motion. No raw client address appears in the extension.

## Known limitations

- The current POC intentionally covers only movement, camera and jump; it has no action bar bindings, targeting, UI, profile, rumble or touchpad support.
- It opens the first SDL-recognised gamepad. Multi-controller selection is deferred.
- It requires an SDL3 x86 runtime alongside the client. A future package can ship/validate that dependency.
- Runtime verification needs a permitted 12340 client and physical controller; this source tree contains neither, so the manual scenario above remains required before declaring gameplay validation complete.
