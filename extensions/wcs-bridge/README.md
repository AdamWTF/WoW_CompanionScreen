# wcs-bridge

In-process WebSocket bridge for WoW 3.3.5a build 12340. It accepts one paired device at `ws://<computer>:18423/wcs`; there is no helper service or asset server.

For same-device use, install the [GitHub Pages PWA](https://adamwtf.github.io/WoW_CompanionScreen/) and use `127.0.0.1`. For a separate LAN device, self-host the PWA over HTTP and use the WoW PC's LAN address.

## Install

Place `wcs-bridge.dll` in `Extensions/wcs-bridge/` and install `addon/WoWCompanionScreen`. The F9 panel and **Display & Connection** page show the endpoint, pairing code, status, paired device, and forget control.

Configuration comes from `wcs-bridge.cfg`, with environment variables taking precedence. Pairing credentials are stored in `pairing.dat` using Windows DPAPI for the current user.

## Protocol

Connect to `/wcs` with RFC 6455 JSON text frames:

```json
{"type":"hello","protocol":1,"client":"wcs"}
```

An unpaired client receives `pairing.required` and sends:

```json
{"type":"pair.request","code":"ABCD-2345","device":{"id":"stable-device-id","name":"My phone"}}
```

Store the token from `pairing.complete`. Returning clients send `{"type":"auth","token":"..."}`. `auth.ok` is followed by `state.snapshot`, then incremental state events.

Input commands are `key.press`, `key.down`, `key.up`, `text.insert`, `pointer.move`, `pointer.click`, `pointer.down`, `pointer.up`, `pointer.scroll`, and `action.press`. Companion slots 1–24 map to WoW action IDs 25–48. Keyboard/text may target WoW in the background; pointer input requires WoW to be foreground.

Transport is plaintext. Keep LAN traffic trusted and never expose port `18423` to the Internet.
