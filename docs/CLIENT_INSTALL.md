# WoW client installation

WoW Companion Screen supports the 32-bit World of Warcraft 3.3.5a client, build 12340. Use a client installation you are permitted to modify and keep an untouched backup.

1. Close World of Warcraft and extract the release ZIP into the client directory, alongside `Wow.exe`.
2. Copy each `.cfg.example` file beside its extension DLL without the `.example` suffix, then adjust it if required.
3. From PowerShell in the client directory, run `./wcs-patcher.exe ./Wow.exe`.
4. Start the game normally. The patcher creates `Wow.exe.orig` before its first modification and safely skips an already-patched executable.

Open WoW Companion Screen with its minimap button, `/wcs`, or `/wowcompanionscreen`. See [the native controller guide](CONTROLLER.md) for the default controls, four action layers, remapping, Smart Interact, UI navigation, supported devices, and tuning options.

The release never contains `Wow.exe`, a patched client, Blizzard assets, or the optional `d3d9.dll` development proxy. Pairing and LAN security guidance is in the repository README.
