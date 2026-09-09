# Changelog

All notable changes to WoW Companion Screen are documented here. Releases use semantic versioning with matching client and PWA versions.

## 1.2.0 - 2026-09-04

### Added

- Show non-raid party members in Thor-optimized Wrath-styled unit frames with class styling, health, class resources, and group status.
- Select party members by tapping their companion-screen frame, or manage the party through confirmed remove, promote, and leave controls.
- Provide Basic and Advanced on-screen keyboard layouts sized for the Thor second screen.

### Changed

- Present party portraits as readable square artwork beside member identity and resource details.
- Move quick actions beside the party roster on wide screens and adapt the roster above them on narrow screens.
- Replace Blizzard party frames only while reduced UI is active and an authenticated companion screen is connected.

### Fixed

- Resolve live spell names and action icons correctly while leaving genuinely unnamed actions uncaptioned.
- Restore Blizzard party frames cleanly after leaving a party without showing a disconnected ghost entry.
- Remove party members reliably through the stock game-thread party operation when Wrath rejects the protected Lua wrapper.

## 1.0.2 - 2026-09-04

### Fixed

- Open the world map from the controller's Select/Back button on WoW 3.3.5a.
- Keep the left-stick camera-view shortcut cycling after the fifth predefined view.
- Restore controller action presses after focus loss or controller reconnection.

## 1.0.1 - 2026-08-31

### Fixed

- Re-synchronize the companion screen from a complete authoritative snapshot every second.
- Follow WoW's live primary action page when forms, stances, or action pages change.
- Open the world map from Select/Back and make menu confirm/cancel orientation configurable.
- Match the add-on's second-screen editor to the PWA's six-column, four-row layout.
- Open all bags by default from the remappable PWA Bags shortcut.
- Recognize lootable and skinnable corpses in Smart Interact.

## 1.0.0 - 2026-08-29

- Initial WoW Companion Screen client and PWA release.
