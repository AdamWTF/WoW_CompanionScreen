# wcs-bridge

WoW Companion Screen bridge is an in-process WoW Companion Screen extension for the 32-bit WoW 3.3.5a build 12340 client.
It exposes a single paired companion device at `ws://<computer>:18423/wcs`; no helper executable,
service, HTTP asset server, or middleware process is used.

For same-device use, including the AYN Thor's second screen, install the GitHub Pages PWA and configure `127.0.0.1`; the hosted app then connects directly to the local bridge. A separate LAN device must use the WoW PC's local IPv4 address and a PWA served over HTTP on that trusted network. See [the complete installation guide](../../docs/CLIENT_INSTALL.md).

## Install

Build the Win32 `wcs-bridge` target and deploy it through the normal `CLIENT_PATH` workflow, or
copy the DLL to `Extensions\wcs-bridge\wcs-bridge.dll`. Copy `addon\WoWCompanionScreen` into the
client's `Interface\AddOns` directory. The F9 diagnostics panel and WoW Companion Screen's **Display & Connection** page show the
endpoint, pairing code, connection state, paired device, and forget control.

Configuration is read from `Extensions\wcs-bridge\wcs-bridge.cfg`, with environment
variables taking precedence. See `wcs-bridge.cfg.example`. Pairing credentials are stored in
`pairing.dat` encrypted for the current Windows user with DPAPI.

## Protocol

Connect to `/wcs` with RFC 6455 JSON text frames. Begin with:

```json
{"type":"hello","protocol":1,"client":"wcs"}
```

An unpaired client receives `pairing.required` and sends:

```json
{"type":"pair.request","code":"ABCD-2345","device":{"id":"stable-device-id","name":"My phone"}}
```

Save the token returned by `pairing.complete`. A returning client sends
`{"type":"auth","token":"..."}`. `auth.ok` is immediately followed by a complete
`state.snapshot`; later changes arrive as the state events documented in the WoW Companion Screen bridge MVP spec.

Required input commands are `key.press`, `key.down`, `key.up`, `text.insert`, `pointer.move`,
`pointer.click`, `pointer.down`, `pointer.up`, `pointer.scroll`, and `action.press`. Companion action slots
1-24 map directly to native WoW action slots 25-48. Keyboard/text messages target WoW's window even
in the background; pointer input is rejected unless WoW is foreground.

The endpoint is plaintext WebSocket transport. A `127.0.0.1` connection remains on the local device. For a LAN connection, pairing prevents casual unauthorized control but does not protect traffic from an attacker able to observe the network; do not expose the port to the public Internet.
