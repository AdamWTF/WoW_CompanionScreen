# wcs-bridge

In-process WebSocket bridge for WoW 3.3.5a build 12340. It accepts one paired device at `ws://<computer>:18423/wcs`; there is no helper service or asset server.

For same-device use, install the [GitHub Pages PWA](https://adamwtf.github.io/WoW_CompanionScreen/) and use `127.0.0.1`. For a separate LAN device, self-host the PWA over HTTP and use the WoW PC's LAN address.

## Install

Place `wcs-bridge.dll` in `Extensions/wcs-bridge/` and install `addon/WoWCompanionScreen`. The F9 panel and **Display & Connection** page show the endpoint, pairing code, status, paired device, and forget control.

Configuration comes from `wcs-bridge.cfg`, with environment variables taking precedence. Pairing credentials are stored in `pairing.dat` using Windows DPAPI for the current user.

## Protocol

Connect to `/wcs` with RFC 6455 JSON text frames:

```json
{"type":"hello","protocol":1,"client":"thor","capabilities":["party","party-management"]}
```

An unpaired client receives `pairing.required` and sends:

```json
{"type":"pair.request","code":"ABCD-2345","device":{"id":"stable-device-id","name":"My phone"}}
```

Store the token from `pairing.complete`. Returning clients send `{"type":"auth","token":"..."}`. `auth.ok` is followed by `state.snapshot`, then incremental state events. Snapshots include `player`, `actions`, and a `party.members` array containing at most `party1`–`party4`; `party.state` replaces that complete array between snapshots. The `party` capability tells the add-on it is safe to replace Blizzard's party frames for this connection.

Input commands are `key.press`, `key.down`, `key.up`, `text.insert`, `pointer.move`, `pointer.click`, `pointer.down`, `pointer.up`, `pointer.scroll`, `action.press`, and `party.select`. Companion slots 1–24 map to WoW action IDs 25–48. `party.select` accepts `member` 1–4 and resolves it through the latest server-owned roster. Keyboard/text and party targeting may reach WoW in the background; pointer input requires WoW to be foreground.

Transport is plaintext. Keep LAN traffic trusted and never expose port `18423` to the Internet.

Party management requires the server hello's `party-management` capability and party metadata: `groupType` (`solo`, `party`, `dungeon-finder`, `raid`), opaque `generation`, and boolean `canRemove`, `canPromote`, `canLeave`. Membership, leadership and group-context changes invalidate the generation; health and target updates do not. Older add-ons omit these fields and expose no management controls.

Authenticated commands are `{"type":"party.remove","member":1,"generation":"…","requestId":"…"}`, `party.promote` with the same fields, and `party.leave` without `member`. Tokens are 1–64 ASCII letters, digits, hyphens or underscores. Results are `{"type":"party.result","requestId":"…","status":"dispatched"}` or a rejection status. Dispatch is not success: only authoritative roster changes confirm the operation. After five seconds without confirmation, request `{"type":"state.request"}`; never automatically retry a management command.

The bounded native queue executes management on the game thread after rechecking live identity, generation and permissions. Removal uses the stock client group method behind `UninviteUnit`, avoiding that Lua wrapper's protected-call gate; promotion and leaving use their stock Wrath APIs through the add-on adapter. Remove/promote require ordinary-party leadership; Dungeon Finder allows leave only; raids reject all three. Requests from disconnected sessions are discarded and duplicate request IDs are rejected within a generation (up to 4096 IDs; further requests fail closed until the generation changes).
