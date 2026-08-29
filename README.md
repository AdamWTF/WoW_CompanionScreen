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

## Downloads and hosted demo

- Download versioned client and PWA archives from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).
- Try the [public PWA demo](https://adamwtf.github.io/WoW_CompanionScreen/?demo). The Pages site is an HTTPS demo/install preview and cannot connect to the current plaintext LAN bridge because browsers block mixed `ws://` content.
- For a live LAN connection, run the PWA container or serve the static PWA release over HTTP on the trusted network.

WoW Companion Screen's current product version is **1.0.0**, independently of the inherited WarcraftXL ABI version. Client and PWA releases can be shipped independently: `client-v1.0.0` publishes the first Windows client bundle, while `pwa-v1.0.0` publishes the first static PWA ZIP and matching container image. Later releases follow the same `client-vX.Y.Z` and `pwa-vX.Y.Z` conventions. Every successful `main` PWA build updates the Pages demo and the `edge` container image; stable PWA tags also update `latest`.

The native runtime still exposes the inherited `wxl::` namespaces and `WXL_*` extension ABI. Those
names are retained deliberately so the upstream SDK boundary remains recognizable and stable; they
are not the product name.

WoW Companion Screen is based on [WarcraftXL 1.1](https://github.com/WarcraftXL/wxl-core/tree/v1.1)
and intentionally retains that release's extension ABI. Modules built for WarcraftXL 1.1 **SHOULD**
therefore work with WoW Companion Screen without modification. Compatibility cannot be guaranteed
for modules that depend on undocumented core internals or target another WarcraftXL release.

## Native controller support

The native gamepad extension provides controller input inside the WoW client—no external key-mapping application is required. Its default gameplay layout is:

| Control | Behavior |
|---|---|
| Left stick / right stick | Character movement and camera control |
| L1 / R1 | Previous and next hostile target |
| L3 / R3 | Cycle camera view and select the next friendly target |
| Start / Select | Toggle the game menu and all bags |
| L2 / R2 | Select Default, L2, R2, or L2+R2 action layers |
| D-pad + face buttons | Eight actions per layer, for 32 assignable positions |

Mappings use WoW's existing action bars and can be edited in game with `/wcs`. Jump and conservative Smart Interact behaviors can be assigned like actions. Supported WoW panels can be navigated with the D-pad, South to confirm, and East to go back; gameplay controls are suppressed while UI navigation is active. XInput works without an additional runtime, while SDL3 adds mapped PlayStation/handheld controllers and DualShock touchpad input.

See the [native controller guide](docs/CONTROLLER.md) for the complete default layout, action-slot mapping, remapping instructions, supported UI panels, device backends, configuration, and runtime safety behavior.

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

For a packaged release, extract the ZIP directly into the client directory and follow [the client installation guide](docs/CLIENT_INSTALL.md). Releases do not include `Wow.exe`, a patched client executable, or the optional `d3d9.dll` development proxy.

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

For a ready-made LAN server, after the GitHub Container Registry package has been made public:

```powershell
docker run --rm -p 8080:80 ghcr.io/adamwtf/wow-companion-screen-pwa:latest
```

Open `http://<WoW-PC-IP>:8080` from the companion device. Live bridge control works in this HTTP/LAN mode, but browsers require HTTPS or localhost for service-worker installation and offline support.

## Release automation setup

The repository uses separate client and PWA GitHub Actions workflows. Before the first deployment, select **GitHub Actions** as the Pages publishing source in repository settings. After the first container publish, set `wow-companion-screen-pwa` to public in its GitHub package settings so users can pull it anonymously. No deployment secrets are required; the workflows use narrowly scoped repository tokens.

## Attribution

WoW Companion Screen would not exist without
[WarcraftXL](https://github.com/WarcraftXL/wxl-core). Our sincere thanks go to its developers and
contributors for sharing the runtime, reverse-engineering work, SDK, and patching foundation that
made this project possible. This repository remains in the WarcraftXL fork network, preserves its Git
history and copyright notices, and is proud to acknowledge where its native foundation came from.
See [NOTICE.md](NOTICE.md) for more detail.

## Legal and license

This is an interoperability project created merely for educational purposes, naturally. Native
releases distribute no Blizzard code or client executable and must be used only with a client and
server you are permitted to modify and access. The PWA distribution includes the tracked icon
library; maintainers are responsible for ensuring that every published asset may legally be
redistributed.

World of Warcraft and Wrath of the Lich King are trademarks of Blizzard Entertainment. WoW Companion
Screen is not affiliated with or endorsed by Blizzard Entertainment.

Released under the **GNU General Public License v3.0**. See [LICENSE](LICENSE). Vendored dependencies
remain under their respective licenses in `deps/`.
