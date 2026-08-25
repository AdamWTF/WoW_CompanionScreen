# wxl-thor-bridge

Thor Bridge is an in-process WarcraftXL extension for the 32-bit WoW 3.3.5a build 12340 client.
It exposes a single paired Thor controller at `ws://<computer>:18423/thor`; no helper executable,
service, HTTP asset server, or middleware process is used.

## Install

Build the Win32 `wxl-thor-bridge` target and deploy it through the normal `CLIENT_PATH` workflow, or
copy the DLL to `Extensions\wxl-thor-bridge\wxl-thor-bridge.dll`. Copy `addon\ThorPad` into the
client's `Interface\AddOns` directory. The WarcraftXL F9 panel and ThorPad's Bridge tab display the
endpoint, pairing code, connection state, paired device, and forget control.

Configuration is read from `Extensions\wxl-thor-bridge\wxl-thor-bridge.cfg`, with environment
variables taking precedence. See `wxl-thor-bridge.cfg.example`. Pairing credentials are stored in
`pairing.dat` encrypted for the current Windows user with DPAPI.

## Protocol

Connect to `/thor` with RFC 6455 JSON text frames. Begin with:

```json
{"type":"hello","protocol":1,"client":"thor"}
```

An unpaired client receives `pairing.required` and sends:

```json
{"type":"pair.request","code":"ABCD-2345","device":{"id":"stable-device-id","name":"My phone"}}
```

Save the token returned by `pairing.complete`. A returning client sends
`{"type":"auth","token":"..."}`. `auth.ok` is immediately followed by a complete
`state.snapshot`; later changes arrive as the state events documented in the Thor Bridge MVP spec.

Required input commands are `key.press`, `key.down`, `key.up`, `text.insert`, `pointer.move`,
`pointer.click`, `pointer.down`, `pointer.up`, `pointer.scroll`, and `action.press`. Thor action slots
1-24 map directly to native WoW action slots 25-48. Keyboard/text messages target WoW's window even
in the background; pointer input is rejected unless WoW is foreground.

The endpoint is plaintext LAN WebSocket transport. Pairing prevents casual unauthorized control but
does not protect traffic from an attacker able to observe the local network; do not expose the port
to the public Internet.
