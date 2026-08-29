# WoW Companion Screen

**Native controller support and a proper second screen for World of Warcraft 3.3.5a (build 12340).**

WoW Companion Screen started as a way of making WoW properly usable on a dual-screen handheld, but it has grown into a more general controller and companion-screen project for 3.3.5a.

It adds native controller support directly to the WoW client, an in-game addon for configuring everything, and a touch-friendly companion web app that can run either on the same device or elsewhere on your local network.

The companion screen can show live character information and action state, trigger abilities and shortcuts, and provide a touchpad and keyboard. There is no separate desktop middleware or cloud service sitting between the app and WoW.

> [!WARNING]
>
> ## Do not use this on Retail WoW or Warden-enabled servers
>
> WoW Companion Screen modifies the WoW client and loads native DLLs directly into the game process.
>
> **Do not use it with Retail World of Warcraft, and do not use it on any private server running Warden or another client-integrity/anti-cheat system.**
>
> This is exactly the sort of client modification those systems are designed to detect. You should assume it **will be flagged and may result in the account being banned**.
>
> This project is intended for 3.3.5a clients and servers where you are explicitly allowed to modify the client.

## Install and set up

WoW Companion Screen currently targets the **32-bit World of Warcraft 3.3.5a client, build 12340**.

I strongly recommend keeping a clean copy of your client before patching anything.

1. Download the latest `wow-companion-screen-client-X.Y.Z.zip` from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).

2. Close WoW and extract the ZIP directly into your client folder alongside `Wow.exe`.

   This installs:

   * `wcs-core.dll`
   * the gamepad extension
   * the second-screen bridge
   * `Interface\AddOns\WoWCompanionScreen`

3. Run `wcs-patcher.exe`.

   The patcher creates `Wow.exe.orig` before modifying anything. If the client is already patched, it will simply skip it.

4. Start WoW normally, log into a character and open WoW Companion Screen using the minimap button or:

   ```text
   /wcs
   ```

5. Open **Display & Connection**.

   The bridge should show as listening. **Enable Controller Support** and **Enable Second Screen** are enabled by default, but both can be turned off from here.

6. If you're using an AYN Thor, open the [hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/) on the second screen.

   From the browser menu, choose **Install app** or **Add to Home screen**, then launch the installed app.

7. Set **WoW PC IPv4 address** to:

   ```text
   127.0.0.1
   ```

   Select **Save & Connect**, then enter the pairing code shown on WoW's **Display & Connection** page or the F9 diagnostics panel.

### How the hosted app works

On something like the AYN Thor, WoW and the PWA are running on the same Windows device.

GitHub Pages only serves the web app itself. Once loaded, the app connects straight back to the local WoW process through:
```text
ws://127.0.0.1:18423/wcs
```

Your game state and controller commands are not being sent through GitHub.

If you're using a phone, tablet or another computer as the companion display, see [Other companion devices](#other-companion-devices).

There is also a more detailed [client installation guide](docs/CLIENT_INSTALL.md) covering manual installation, configuration, troubleshooting and updating.

## What's in the project?

| Component                  | What it does                                                                                                  |
| -------------------------- | ------------------------------------------------------------------------------------------------------------- |
| `wcs-core.dll`             | Core in-process runtime, hook engine and extension loader. Also exposes the inherited WarcraftXL SDK surface. |
| `wcs-patcher.exe`          | Patches the 32-bit WoW executable so `wcs-core.dll` is loaded with the client.                                |
| `wcs-gamepad.dll`          | Handles controller detection, gameplay controls, Smart Interact and controller-driven UI navigation.          |
| `wcs-bridge.dll`           | Runs the local WebSocket bridge between WoW and the companion app.                                            |
| `addon/WoWCompanionScreen` | In-game configuration, controller mappings, action overlays and connection state.                             |
| `wcs-web`                  | The installable companion web app.                                                                            |

## Downloads and hosted app

* Download client and PWA releases from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).
* Open the [hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/) when running the companion screen on the same device as WoW. Use `127.0.0.1` as the WoW PC address.
* You can also try the [PWA demo](https://adamwtf.github.io/WoW_CompanionScreen/?demo) without installing anything or running WoW.
* If the companion screen is another physical device, you can run the PWA container or serve the static PWA release over HTTP on your local network.

WoW Companion Screen currently uses its own **1.0.0** versioning separately from the WarcraftXL ABI it is built on.

Client and PWA releases are also versioned separately:
```text
client-v1.0.0
pwa-v1.0.0
```

Future releases follow the same:
```text
client-vX.Y.Z
pwa-vX.Y.Z
```

Successful PWA builds from `main` update the hosted GitHub Pages version and the `edge` container image. Stable PWA releases also update `latest`.

## WarcraftXL compatibility

The native side of WoW Companion Screen is built on [WarcraftXL 1.1](https://github.com/WarcraftXL/wxl-core/tree/v1.1).

I've deliberately kept the existing `wxl::` namespaces and `WXL_*` extension ABI rather than needlessly renaming everything underneath it.

As a result, extensions built against the WarcraftXL 1.1 ABI **should** also work with WoW Companion Screen without needing changes.

Obviously, I can't promise compatibility with extensions that rely on undocumented WarcraftXL internals or target a different version.

## Native controller support

Controller support runs directly inside the WoW client, so you don't need something like JoyToKey or another external remapping application running alongside it.

The default layout is:

| Control              | Action                                                                    |
| -------------------- | ------------------------------------------------------------------------- |
| Left stick           | Character movement and strafing                                           |
| Right stick          | Camera control                                                            |
| L1 / R1              | Previous / next hostile target                                            |
| L3 / R3              | Cycle camera view / next friendly target                                  |
| Start / Select       | Game menu / bags                                                          |
| L2 / R2              | Switch between Default, L2, R2 and L2+R2 action layers                    |
| D-pad + face buttons | Eight actions per layer, giving 32 directly assignable controller actions |

The action mappings use WoW's existing action bars and can be configured in-game through:
```text
/wcs
```

Jump and Smart Interact can also be placed onto the controller like normal actions rather than being permanently tied to a particular button.

WoW panels that support controller navigation can be controlled using the D-pad, with **South** used to confirm and **East** used to go back. Gameplay controls are temporarily suppressed while navigating the UI so the same inputs don't also trigger actions in-game.

XInput controllers work without any additional runtime.

SDL3 adds support for mapped PlayStation controllers, handheld controllers and DualShock touchpad input.

See the [native controller guide](docs/CONTROLLER.md) for the complete layouts, action-slot mappings, remapping options, supported interfaces, controller backends and configuration.

## Build the native components

You'll need:

* CMake 3.20 or newer
* Visual Studio 2022 with the Win32 C++ toolchain
* A WoW 3.3.5a build 12340 client that you're allowed to modify

To build everything and deploy it directly into a client:
```powershell
.\build.ps1 -ClientPath "D:\Path\To\Client"
```

You can also have the build script patch `Wow.exe` automatically:
```powershell
.\build.ps1 -ClientPath "D:\Path\To\Client" -AutoPatch
```

The patcher creates `Wow.exe.orig` the first time it modifies the executable.

Again: keep a clean client backup. There's really no reason not to.

To build without deploying:
```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release --parallel
```

Copy the addon from:
```text
addon/WoWCompanionScreen
```

to:
```text
Interface/AddOns/WoWCompanionScreen
```

Example configuration files are available in `docs/wcs-core.cfg.example` and alongside the individual extensions.

Packaged releases don't include `Wow.exe`, a pre-patched WoW executable or the optional `d3d9.dll` development proxy.

See the [client installation guide](docs/CLIENT_INSTALL.md) for the full packaged installation process.

## Other companion devices

The easiest setup is when WoW and the companion screen are running on the same device, but they don't have to be.

You can also use a phone, tablet, laptop or pretty much anything with a modern browser.

There is one catch: an HTTPS-hosted page generally can't open a plaintext `ws://` WebSocket connection to another machine on your LAN.

For a separate device, run the PWA locally over HTTP instead.

Once the GitHub Container Registry package is public, the container can be run on the WoW PC or another machine on the network:
```powershell
docker run --rm -p 8080:80 ghcr.io/adamwtf/wow-companion-screen-pwa:latest
```

Then open:
```text
http://<PWA-host-IP>:8080
```

from the companion device.

Set **WoW PC IPv4 address** to the machine actually running WoW, for example:
```text
192.168.1.50
```

The bridge runs on TCP port `18423`.

**Keep this on your trusted local network. Do not expose port 18423 directly to the Internet.**

Pairing makes sure another random device can't immediately control the client, but the WebSocket transport itself is still plaintext.

When served this way, live control works normally. Some installable/offline PWA features may not because browsers generally require HTTPS or localhost for those features.

## Develop the companion web app
```powershell
cd wcs-web
npm install
npm run dev
```

Open:
```text
http://localhost:3000/?demo
```

to run the UI with demo data and no WoW client connected.

A production static build is generated in:
```text
wcs-web/out
```

using:
```powershell
npm run build
```

For a live connection, the browser connects to:
```text
ws://<WoW-PC-IP>:18423/wcs
```

If the PWA and WoW are running on the same machine, just use:
```text
127.0.0.1
```

## Release automation

The repository has separate GitHub Actions workflows for the native client and PWA.

Before deploying the PWA for the first time, set **GitHub Actions** as the Pages publishing source in the repository settings.

After the first container image has been published, make the `wow-companion-screen-pwa` package public in the GitHub package settings if you want people to be able to pull it anonymously.

No custom deployment secrets are currently required.

## Attribution

WoW Companion Screen is built on top of [WarcraftXL](https://github.com/WarcraftXL/wxl-core).

A huge amount of the difficult groundwork — the runtime, reverse-engineering, SDK and patching system — came from that project, and this wouldn't exist without it.

This repository stays within the WarcraftXL fork network and keeps its Git history and original copyright notices intact.

See [NOTICE.md](NOTICE.md) for the details.

## Legal and license

WoW Companion Screen does not contain or distribute Blizzard's client, a modified `Wow.exe`, or other Blizzard-owned code.

You are responsible for making sure you're allowed to modify the client and connect to the server you're using.

And, because it's important enough to say twice:

**Do not use WoW Companion Screen with Retail World of Warcraft or a private server running Warden or another client-integrity/anti-cheat system. The project modifies and injects native code into the WoW client and should be assumed to be detectable. Doing so risks the account being flagged or banned.**

World of Warcraft and Wrath of the Lich King are trademarks of Blizzard Entertainment.

WoW Companion Screen is not affiliated with or endorsed by Blizzard Entertainment.

WoW Companion Screen is released under the **GNU General Public License v3.0**. See [LICENSE](LICENSE).

Vendored dependencies remain under their own respective licences in `deps/`.
