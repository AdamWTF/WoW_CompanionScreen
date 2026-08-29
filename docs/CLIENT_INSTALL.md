# Installation and setup

WoW Companion Screen is built for the **32-bit World of Warcraft 3.3.5a client, build 12340**.

Keep an untouched copy of your client before you start. It makes updates, troubleshooting and removing the mod much easier.

> [!WARNING]
>
> WoW Companion Screen modifies the client and loads native DLLs into the game process. Do not use it with Retail WoW or any private server running Warden or another client-integrity/anti-cheat system. Assume it can be detected and that using it there could get the account banned.

## Install the mod

1. Download the latest `wow-companion-screen-client-X.Y.Z.zip` from [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases).

   If you're using the hosted companion app, you do not need the separate PWA ZIP.

2. Close WoW.

3. Extract the **contents** of the client ZIP directly into the WoW folder alongside `Wow.exe`.

   You should end up with:

   ```text
   Wow.exe
   wcs-core.dll
   wcs-patcher.exe
   Extensions\wcs-gamepad\wcs-gamepad.dll
   Extensions\wcs-bridge\wcs-bridge.dll
   Interface\AddOns\WoWCompanionScreen\WoWCompanionScreen.toc
   ```

   If those files are inside another `wow-companion-screen-client-X.Y.Z` folder, you've extracted the ZIP one level too deep.

4. Run `wcs-patcher.exe`.

   Before changing anything, the patcher saves the original executable as `Wow.exe.orig`. Running it again is safe; it will detect an already-patched client and skip it.

5. Start WoW and log into a character.

6. Open WoW Companion Screen from the minimap button or type:

   ```text
   /wcs
   ```

   `/wowcompanionscreen` works as well.

You don't need to create any configuration files for a normal setup. The included defaults are ready to use.

If you do want to change advanced settings:

* copy `Extensions\wcs-gamepad\wcs-gamepad.cfg.example` to `wcs-gamepad.cfg`
* copy `Extensions\wcs-bridge\wcs-bridge.cfg.example` to `wcs-bridge.cfg`
* copy `docs\wcs-core.cfg.example` alongside `Wow.exe` as `wcs-core.cfg`

## Set up the controller

Open **Display & Connection** in `/wcs`.

**Enable Controller Support** is on by default. If the controller is connected but nothing happens, press **F9** and check which backend detected it.

Open **Controller Mapping** to set up the four action layers. Pick a layer, then drag spells, items, macros or existing WoW actions into its eight slots. You can also place **Jump** and **Interact** like normal actions. Right-click a slot to clear it.

Mappings can only be changed out of combat.

See the [controller guide](CONTROLLER.md) for the full layout, supported devices, UI navigation and advanced settings.

## Set up the AYN Thor second screen

1. In `/wcs`, open **Display & Connection**.

   Make sure **Enable Second Screen** is selected and the bridge says it is listening.

2. Open **Second Screen** and drag actions into the 24 available slots. These use WoW's normal action slots 25–48, so WoW saves the assignments.

3. On the Thor's second screen, open the [hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/).

4. Use the browser menu to choose **Install app** or **Add to Home screen**, then open the installed app.

5. Select **Set up connection** and enter:

   ```text
   127.0.0.1
   ```

6. Select **Save & Connect**.

7. Enter the pairing code shown on WoW's **Display & Connection** page or the F9 diagnostics panel, then select **Pair Device**.

Although the app is loaded from GitHub Pages, the live connection stays on the Thor. The app connects straight to WoW through:

```text
ws://127.0.0.1:18423/wcs
```

GitHub does not receive your game state or control input.

The bridge accepts one companion device at a time. To use a different one, forget the current device in WoW and pair again.

## Use a phone, tablet or another computer

`127.0.0.1` only works when the companion app and WoW are on the same machine.

For another physical device, run the PWA over HTTP on your trusted local network. You can serve the static PWA release yourself or run the container:

```powershell
docker run --rm -p 8080:80 ghcr.io/adamwtf/wow-companion-screen-pwa:latest
```

Open `http://<PWA-host-IP>:8080` on the companion device, then enter the local IPv4 address of the PC running WoW, for example `192.168.1.50`.

Allow inbound TCP port `18423` on private networks if Windows Firewall asks. Do not forward that port or expose it to the Internet. Pairing prevents casual access, but the WebSocket traffic is not encrypted.

## Update the mod

1. Close WoW.
2. Extract the newer client ZIP into the same WoW folder and replace the old WCS files.
3. Run the patcher again.

Your addon settings and paired device are kept unless you delete WoW's saved variables or `Extensions\wcs-bridge\pairing.dat`.

## Remove the mod

Close WoW, then replace `Wow.exe` with the untouched `Wow.exe.orig` created by the patcher.

To remove the remaining files, delete:

* `wcs-core.dll`
* `Extensions\wcs-gamepad`
* `Extensions\wcs-bridge`
* `Interface\AddOns\WoWCompanionScreen`

## Troubleshooting

### The addon doesn't appear

Check that `Interface\AddOns\WoWCompanionScreen\WoWCompanionScreen.toc` exists and that you're using client build 12340. The most common mistake is extracting the release into an extra folder.

### The controller doesn't respond

Log into a character first. Controller input is deliberately disabled on login, realm, character-selection and loading screens. Check **Enable Controller Support**, then press **F9** to see which backend is active.

### The second screen can't connect

Keep WoW running and make sure the bridge says it is listening.

On an AYN Thor, use `127.0.0.1`. On another device, use the WoW PC's LAN address and check the private-network firewall rule for TCP port `18423`.

### Pairing stopped working

This can happen after clearing browser data or reinstalling the PWA. Forget the paired device in WoW, reconnect and enter the new pairing code.

Packaged releases do not contain `Wow.exe`, a pre-patched executable, Blizzard assets or the optional `d3d9.dll` development proxy.
