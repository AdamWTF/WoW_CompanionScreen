# WoW Companion Screen

**Controller-first play and a local companion display for World of Warcraft 3.3.5a (build 12340).**

WoW Companion Screen combines native controller support, an in-game configuration add-on, and a
touch-friendly web display. The web app connects directly to the game over the local network, showing
live character and action state while providing action buttons, shortcuts, a touchpad, and keyboard
input. No cloud service or desktop middleware is required.

## Components

| Component | Purpose |
|---|---|
| `wcs-core.dll` | In-process runtime, hook engine, extension loader, and inherited WarcraftXL SDK surface. |
| `wcs-patcher.exe` | Adds the core DLL import to a copy of the 32-bit client executable. |
| `wcs-gamepad.dll` | Controller discovery, gameplay input, Smart Interact, and controller UI navigation. |
| `wcs-bridge.dll` | Paired LAN WebSocket bridge between the game and companion web app. |
| `addon/WoWCompanionScreen` | In-game settings, mappings, action overlay, and bridge state. |
| `wcs-web` | Installable static companion web app. |

The native runtime still exposes the inherited `wxl::` namespaces and `WXL_*` extension ABI. Those
names are retained deliberately so the upstream SDK boundary remains recognizable and stable; they
are not the product name.

## Build the native components

Requirements:

- CMake 3.20 or newer
- Visual Studio 2022 with a Win32 C++ toolchain
- A legally obtained 3.3.5a (12340) client

To build and deploy all native components into a client directory:

```powershell
.\build.ps1 -ClientPath "D:\Path\To\Client"
```

Add `-AutoPatch` to patch that client's `Wow.exe` after building. The patcher creates `Wow.exe.orig`
the first time it changes the executable. Work on a copy and keep an untouched backup.

For a build without deployment:

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release --parallel
```

Copy `addon/WoWCompanionScreen` to `Interface/AddOns/WoWCompanionScreen` in the client. Configuration
examples are provided in `docs/wcs-core.cfg.example` and beside each extension.

## Run the companion web app

```powershell
cd wcs-web
npm install
npm run dev
```

Open `http://localhost:3000/?demo` to use representative data without a running game client. A
production static export is generated in `wcs-web/out` by `npm run build`.

The browser connects to `ws://<WoW-PC-IP>:18423/wcs`. Keep port 18423 on a trusted LAN and never
expose it to the public Internet; pairing authenticates a device but the transport is plaintext.

## Attribution

WoW Companion Screen would not exist without
[WarcraftXL](https://github.com/WarcraftXL/wxl-core). Our sincere thanks go to its developers and
contributors for sharing the runtime, reverse-engineering work, SDK, and patching foundation that
made this project possible. This repository remains in the WarcraftXL fork network, preserves its Git
history and copyright notices, and is proud to acknowledge where its native foundation came from.
See [NOTICE.md](NOTICE.md) for more detail.

## Legal and license

This is an interoperability project created merely for educational purposes, naturally. It
distributes no Blizzard code, client executable, or game assets and must be used only with a client
and server you are permitted to modify and access.

World of Warcraft and Wrath of the Lich King are trademarks of Blizzard Entertainment. WoW Companion
Screen is not affiliated with or endorsed by Blizzard Entertainment.

Released under the **GNU General Public License v3.0**. See [LICENSE](LICENSE). Vendored dependencies
remain under their respective licenses in `deps/`.
