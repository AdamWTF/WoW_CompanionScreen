# WoW Companion Screen PWA

Touch-friendly companion app for WoW Companion Screen.

## Develop

From `wcs-web`:

```powershell
npm ci
npm test
npm run typecheck
npm run dev
```

Open `http://localhost:3000/?demo` for representative data without WoW. Development uses `.next-dev/`, independently of production builds. `npm run build` creates the static site in `out/`.

For Thor layout checks, open companion **Settings → Interface → Display diagnostics** in the dev app to read its CSS viewport, display density, visible height, scale, and active layout. Start with the measured 537×468 CSS viewport, then check 1240×1080, 620×540, and 496×432: party members belong on the left with square portraits, full-width bars, a 6×4 action grid, and visible bottom navigation. The 390×844 phone fallback uses a two-column party roster and 4×6 actions. Repeat at UI scales 0.85 and 1.15 and verify on the device itself.

## GitHub Pages

The maintained deployment is the [hosted companion app](https://adamwtf.github.io/WoW_CompanionScreen/); append `?demo` for demo mode. Install it on the same Windows device as WoW and use `127.0.0.1`.

Pages builds use `/WoW_CompanionScreen` as their base path. The app then connects directly to `ws://127.0.0.1:18423/wcs`; game state and controls are not relayed through GitHub.

## Self-host on a LAN

A separate device cannot normally connect from the hosted HTTPS app to a plaintext LAN WebSocket. Build and serve the PWA over HTTP on a trusted network.

The retained Docker setup is source-only and unsupported as a published distribution:

```powershell
docker build -t wcs-pwa-local .
docker run --rm -p 8080:80 wcs-pwa-local
```

Open `http://<PWA-host-IP>:8080`, then configure the WoW PC's LAN IPv4 address. Never expose TCP port `18423` to the Internet. HTTP supports live control, but install/offline features may require HTTPS or localhost.

## WoW icons

Place a legally sourced 3.3.5a icon library in `public/assets/wow-icons` using lowercase WebP filenames. Missing icons use the neutral fallback.
