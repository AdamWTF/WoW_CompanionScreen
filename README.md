# WoW Companion Screen

Native controller support and a touch-friendly second screen for the 32-bit World of Warcraft 3.3.5a client, build 12340.

> [!WARNING]
> This project modifies the WoW client and loads native DLLs. Do not use it with Retail WoW or servers running Warden or another client-integrity system; assume it is detectable and may result in a ban.

## Quickstart

1. Download `wow-companion-screen-client-X.Y.Z.zip` from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).
2. Extract the ZIP contents beside `Wow.exe` in your installation folder.
3. Run `wcs-patcher.exe`. It creates `Wow.exe.orig` before patching.
4. Start WoW, log into a character, and open the settings with the minimap button or `/wcs`.
5. Under **Display & Connection**, enable the controller and second screen.
6. Open the [hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/) on the same Windows device, install it from the browser, and set the WoW PC address to `127.0.0.1`.
7. Enter the pairing code shown in WoW.

Keep an untouched client copy. See the [installation guide](docs/CLIENT_INSTALL.md) for configuration, updating, removal, LAN use, and troubleshooting.

## Features

- Native movement, camera, targeting, layered actions, Jump, Smart Interact, and configurable UI navigation.
- A paired companion screen with 24 actions, character state, keyboard, touchpad, and shortcuts.
- Direct local communication with WoW; the hosted app does not relay game state through GitHub.

| Component | Purpose |
|---|---|
| `wcs-core.dll` | In-process runtime, hooks, and extension loader |
| `wcs-patcher.exe` | Adds the core DLL to the supported client |
| `wcs-gamepad.dll` | Controller input and UI navigation |
| `wcs-bridge.dll` | Local WebSocket bridge for one paired companion device |
| `addon/WoWCompanionScreen` | In-game configuration and action mappings |
| `wcs-web` | GitHub Pages companion app |

See the [controller guide](docs/CONTROLLER.md) for layouts, mappings, supported backends, and advanced settings. The [PWA demo](https://adamwtf.github.io/WoW_CompanionScreen/?demo) works without WoW.

## License and attribution

WoW Companion Screen is based on [WarcraftXL](https://github.com/WarcraftXL/wxl-core) and retains its extension ABI. See [NOTICE.md](NOTICE.md) for attribution and [LICENSE](LICENSE) for GPLv3 terms.

World of Warcraft and Wrath of the Lich King are trademarks of Blizzard Entertainment. This project is not affiliated with or endorsed by Blizzard Entertainment.
