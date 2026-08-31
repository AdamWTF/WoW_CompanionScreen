# Installation and setup

For the 32-bit World of Warcraft 3.3.5a client, build 12340. Keep an untouched client copy.

> [!WARNING]
> The project modifies the client and loads native DLLs. Do not use it with Retail WoW or servers running Warden or another client-integrity system.

## Install

1. Download `wow-companion-screen-client-X.Y.Z.zip` from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).
2. Close WoW and extract the ZIP contents beside `Wow.exe`.
3. Confirm these paths exist:

   ```text
   wcs-core.dll
   wcs-patcher.exe
   Extensions\wcs-gamepad\wcs-gamepad.dll
   Extensions\wcs-bridge\wcs-bridge.dll
   Interface\AddOns\WoWCompanionScreen\WoWCompanionScreen.toc
   ```

4. Run `wcs-patcher.exe`. It saves the original as `Wow.exe.orig` and safely skips an already-patched client.
5. Start WoW, log into a character, and open the settings with the minimap button, `/wcs`, or `/wowcompanionscreen`.

The defaults require no configuration files. For advanced settings, copy the relevant `.cfg.example` file without the `.example` suffix. Copy `docs\wcs-core.cfg.example` beside `Wow.exe` as `wcs-core.cfg`.

## Controller

Controller support is enabled under **Display & Connection**. Configure the four layers under **Controller Mapping** by dragging actions into the eight slots; right-click to clear a slot. Mappings can only change out of combat.

See the [controller guide](CONTROLLER.md) for layouts, backends, UI navigation, Jump, Smart Interact, and advanced settings. Press **F9** for native diagnostics.

## Same-device companion screen

1. Under **Display & Connection**, enable the second screen and confirm the bridge is listening.
2. Under **Second Screen**, assign its 24 slots. They map to WoW action IDs 25–48.
3. Open and install the [hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/) on the same Windows device.
4. Set the WoW PC address to `127.0.0.1`, connect, and enter the pairing code shown in WoW or the F9 panel.

The app connects directly to `ws://127.0.0.1:18423/wcs`. GitHub only serves the app files. The bridge accepts one paired device at a time; forget the current device in WoW before pairing another.

## Separate LAN device

The hosted HTTPS app cannot connect to a plaintext WebSocket on another LAN machine. Build and serve the PWA from source over HTTP instead; see [`wcs-web/README.md`](../wcs-web/README.md).

Open the local PWA on the companion device and enter the WoW PC's LAN IPv4 address. Allow inbound TCP port `18423` on private networks if required. Never forward or expose this plaintext endpoint to the Internet.

## Update or remove

To update, close WoW, replace the WCS files with a newer client ZIP, and run the patcher again. Addon settings and `Extensions\wcs-bridge\pairing.dat` remain unless deleted.

To remove WCS, close WoW, restore `Wow.exe.orig` as `Wow.exe`, then remove:

- `wcs-core.dll`
- `Extensions\wcs-gamepad`
- `Extensions\wcs-bridge`
- `Interface\AddOns\WoWCompanionScreen`

## Troubleshooting

- **Addon missing:** confirm the `.toc` path above, client build 12340, and that the ZIP was not extracted into an extra directory.
- **Controller inactive:** log into a character, check **Enable Controller Support**, and inspect the active backend with F9.
- **Second screen offline:** keep WoW running, confirm the bridge is listening, and use `127.0.0.1` only for a same-device connection.
- **Pairing fails after browser data was cleared:** forget the paired device in WoW, reconnect, and enter the new code.

Client releases exclude `Wow.exe`, pre-patched executables, Blizzard assets, and the optional development `d3d9.dll` proxy.
