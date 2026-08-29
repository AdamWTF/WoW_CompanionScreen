# WoW Companion Screen

**Controller-first play and a second-screen companion display for World of Warcraft 3.3.5a (build 12340).**

WoW Companion Screen combines native controller support, an in-game configuration add-on, and a
touch-friendly web display. The web app connects directly to the game on the same device or over the local network, showing
live character and action state while providing action buttons, shortcuts, a touchpad, and keyboard
input. No cloud service or desktop middleware is required.

## Install and set up

WoW Companion Screen supports the 32-bit World of Warcraft 3.3.5a client, build 12340. Use a client installation you are permitted to modify, and keep an untouched backup.

1. Download the latest `wow-companion-screen-client-X.Y.Z.zip` from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).
2. Close World of Warcraft and extract the contents of the ZIP directly into the client folder, alongside `Wow.exe`. This installs `wcs-core.dll`, the gamepad and bridge extensions, and `Interface\AddOns\WoWCompanionScreen` in one step.
3. Open PowerShell in the client folder and run:

   ```powershell
   .\wcs-patcher.exe .\Wow.exe
   ```

   The patcher creates `Wow.exe.orig` before changing the client and safely skips a client that is already patched.
4. Start WoW normally, enter the world, and open the mod from its minimap button or by typing `/wcs`.
5. Open **Display & Connection** and confirm that the bridge is listening. **Enable Controller Support** and **Enable Second Screen** are on by default and can be changed here.
6. On the AYN Thor, open [the hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/) on the second screen. Use the browser menu to choose **Install app** or **Add to Home screen**, then open the installed app.
7. In the companion app, set **WoW PC IPv4 address** to `127.0.0.1` and select **Save & Connect**. Enter the pairing code shown on WoW's **Display & Connection** page (or the F9 diagnostics panel).

The hosted app works live on an AYN Thor because WoW and the second-screen PWA run on the same Windows device: the PWA assets come from GitHub Pages, while its bridge connection goes straight back to the local WoW process through `ws://127.0.0.1:18423/wcs`. It does not send game state or controls through GitHub.

For a phone, tablet, or other separate companion device, see [Other companion devices](#other-companion-devices). For configuration, troubleshooting, manual installation, and updates, see the [complete client installation guide](docs/CLIENT_INSTALL.md).

## Components

| Component | Purpose |
|---|---|
| `wcs-core.dll` | In-process runtime, hook engine, extension loader, and inherited WarcraftXL SDK surface. |
| `wcs-patcher.exe` | Adds the core DLL import to a copy of the 32-bit client executable. |
| `wcs-gamepad.dll` | Controller discovery, gameplay input, Smart Interact, and controller UI navigation. |
| `wcs-bridge.dll` | Paired LAN WebSocket bridge between the game and companion web app. |
| `addon/WoWCompanionScreen` | In-game settings, mappings, action overlay, and bridge state. |
| `wcs-web` | Installable static companion web app. |

## Downloads and hosted app

- Download versioned client and PWA archives from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).
- Open or install the [public PWA](https://adamwtf.github.io/WoW_CompanionScreen/) for live use on the same device as WoW. Set its WoW PC address to `127.0.0.1`.
- Try the [public PWA demo](https://adamwtf.github.io/WoW_CompanionScreen/?demo) without installing the mod or running WoW.
- For live use from a separate device, run the PWA container or serve the static PWA release over HTTP on the trusted local network.

WoW Companion Screen's current product version is **1.0.0**, independently of the inherited WarcraftXL ABI version. Client and PWA releases can be shipped independently: `client-v1.0.0` publishes the first Windows client bundle, while `pwa-v1.0.0` publishes the first static PWA ZIP and matching container image. Later releases follow the same `client-vX.Y.Z` and `pwa-vX.Y.Z` conventions. Every successful `main` PWA build updates the hosted Pages app and the `edge` container image; stable PWA tags also update `latest`.

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

## Other companion devices

An HTTPS-hosted page generally cannot open the bridge's plaintext `ws://` connection to another device on the LAN. If the companion display is a phone, tablet, or separate computer, host the PWA over HTTP on the trusted LAN and connect it to the WoW PC's local IPv4 address.

After the GitHub Container Registry package has been made public, the ready-made container can run on the WoW PC or another machine:

```powershell
docker run --rm -p 8080:80 ghcr.io/adamwtf/wow-companion-screen-pwa:latest
```

Open `http://<PWA-host-IP>:8080` from the companion device, then set **WoW PC IPv4 address** to the WoW PC's address, for example `192.168.1.50`. Keep TCP port 18423 on a trusted LAN and never expose it to the public Internet; pairing authenticates a device, but the transport is plaintext.

HTTP LAN hosting supports live bridge control. Installable/offline PWA features normally require HTTPS or localhost, so they may not be available to a separate device in this mode.

## Develop the companion web app

```powershell
cd wcs-web
npm install
npm run dev
```

Open `http://localhost:3000/?demo` to use representative data without a running game client. A
production static export is generated in `wcs-web/out` by `npm run build`.

The browser connects to `ws://<WoW-PC-IP>:18423/wcs`; use `127.0.0.1` when the PWA and WoW run on the same device.

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
