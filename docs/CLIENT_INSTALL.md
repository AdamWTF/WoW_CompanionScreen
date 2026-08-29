# Installation and setup

WoW Companion Screen supports the 32-bit World of Warcraft 3.3.5a client, build 12340. Use a client installation you are permitted to modify and keep an untouched backup.

## Install the mod

1. Download the latest `wow-companion-screen-client-X.Y.Z.zip` from the repository's [GitHub Releases](https://github.com/AdamWTF/WoW_CompanionScreen/releases). You do not need the separate PWA ZIP when using the hosted app.
2. Close World of Warcraft.
3. Extract the **contents** of the client ZIP directly into the client folder, alongside `Wow.exe`. After extraction, check that these paths exist:

   ```text
   Wow.exe
   wcs-core.dll
   wcs-patcher.exe
   Extensions\wcs-gamepad\wcs-gamepad.dll
   Extensions\wcs-bridge\wcs-bridge.dll
   Interface\AddOns\WoWCompanionScreen\WoWCompanionScreen.toc
   ```

   Avoid creating an extra `wow-companion-screen-client-X.Y.Z` folder around these files.
4. Open PowerShell in the client folder and run:

   ```powershell
   .\wcs-patcher.exe .\Wow.exe
   ```

   The patcher creates `Wow.exe.orig` before its first modification and safely skips an already-patched client. The release does not contain `Wow.exe` or a pre-patched executable.
5. Start WoW normally and enter the world. Open WoW Companion Screen from its minimap button or by typing `/wcs` or `/wowcompanionscreen`.

The supplied defaults work without creating configuration files. Advanced users can copy `Extensions\wcs-gamepad\wcs-gamepad.cfg.example` to `wcs-gamepad.cfg`, copy `Extensions\wcs-bridge\wcs-bridge.cfg.example` to `wcs-bridge.cfg`, or copy `docs\wcs-core.cfg.example` beside `Wow.exe` as `wcs-core.cfg`, then edit only the required settings.

## Set up the controller and second screen

1. In `/wcs`, open **Display & Connection**. Confirm that **Enable Controller Support** and **Enable Second Screen** are selected and that the bridge says it is listening.
2. Assign controller actions on **Controller Mapping**, and assign the 24 companion-screen actions on **Second Screen**. Drag spells, items, macros, or existing WoW actions into the displayed slots while out of combat.
3. On the AYN Thor's second screen, open [the hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/).
4. From the browser menu, choose **Install app** or **Add to Home screen**, then launch WoW Companion Screen as an installed app.
5. Select **Set up connection**. Enter `127.0.0.1` for **WoW PC IPv4 address**, then select **Save & Connect**.
6. Enter the pairing code shown on WoW's **Display & Connection** page or F9 diagnostics panel, then select **Pair Device**.

This works even though the PWA is installed from GitHub Pages. GitHub supplies the app files over HTTPS, but the live connection is made locally from the PWA to the WoW process at `ws://127.0.0.1:18423/wcs`; game state and controls do not pass through GitHub.

Only one companion device can be connected at a time. To replace it, forget the paired device in WoW and pair the new one. Pairing data is stored in `Extensions\wcs-bridge\pairing.dat` for the current Windows user.

## Use a separate phone or tablet

The GitHub-hosted PWA's localhost connection only reaches WoW when the PWA and WoW run on the same device. An HTTPS page will generally be blocked from opening the bridge's plaintext `ws://` connection to a different LAN device.

For a separate phone, tablet, or computer, serve the static PWA release or container over HTTP on the trusted LAN. Open that LAN-hosted URL on the companion device and enter the WoW PC's local IPv4 address, such as `192.168.1.50`. Ensure the Windows firewall permits inbound TCP port 18423 on private networks only. Never forward or expose port 18423 to the Internet.

## Update or remove

To update, close WoW, extract the newer client ZIP into the same client folder, allow it to replace the existing WCS files, and run the patcher again. Your WoW add-on settings and bridge pairing are retained unless you delete `WTF` saved variables or `Extensions\wcs-bridge\pairing.dat`.

To restore the original executable, close WoW and replace `Wow.exe` with the untouched `Wow.exe.orig` created by the patcher. Remove the WCS DLLs, `Extensions\wcs-gamepad`, `Extensions\wcs-bridge`, and `Interface\AddOns\WoWCompanionScreen` if you also want to remove the mod files.

## Troubleshooting

- **The mod does not appear:** confirm the add-on TOC path shown above, that the client is build 12340, and that the ZIP was not extracted into an extra folder.
- **The controller does not respond:** enter the world first, check **Enable Controller Support**, and press F9 to inspect the detected backend. Login and character-selection screens are diagnostic-only.
- **The second screen cannot connect:** keep WoW running, confirm that the bridge is listening, and use `127.0.0.1` on the AYN Thor. For a separate device, use the WoW PC's LAN address and check the private-network firewall rule for TCP port 18423.
- **Pairing fails after reinstalling or clearing browser data:** forget the paired device in WoW, reconnect, and enter the newly displayed code.

See [the native controller guide](CONTROLLER.md) for the default controls, four action layers, remapping, Smart Interact, UI navigation, supported devices, and tuning options.

The release never contains Blizzard assets or the optional `d3d9.dll` development proxy.
