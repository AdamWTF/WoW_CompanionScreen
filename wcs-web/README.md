# WoW Companion Screen PWA

Static Next.js UI for the WoW Companion Screen bridge.

```powershell
npm install
npm run dev
```

For UI work without a running WoW client, open `http://localhost:3000/?demo`. Demo mode skips the bridge connection and fills the PWA with representative player and action data in both development and static builds.

Production output is generated in `out/` by `npm run build`. Build the included Dockerfile for a lightweight Nginx image.

The hosted app is deployed to `https://adamwtf.github.io/WoW_CompanionScreen/`; add `?demo` to use representative data without WoW. GitHub Pages uses the `/WoW_CompanionScreen` build base path; root-hosted static and Docker builds continue to use `/`.

The GitHub-hosted PWA supports live use when it runs on the same Windows device as WoW, including the AYN Thor's second screen. Install it from the browser and set the WoW PC IPv4 address to `127.0.0.1`. The app files come from GitHub Pages, while the live WebSocket connection remains local at `ws://127.0.0.1:18423/wcs`; game state and controls do not pass through GitHub.

For a separate phone, tablet, or computer, the HTTPS-hosted app generally cannot open a plaintext `ws://` bridge on another LAN device. Use the root-hosted release ZIP or the container on the trusted LAN:

```powershell
docker run --rm -p 8080:80 ghcr.io/adamwtf/wow-companion-screen-pwa:latest
```

Open `http://<PWA-host-IP>:8080` on the companion device and configure the WoW PC's LAN IPv4 address. HTTP LAN hosting supports the live bridge, while service-worker installation and offline behavior require HTTPS or localhost.

The browser connects directly to `ws://<WoW-PC-IP>:18423/wcs`. Use `127.0.0.1` for same-device operation. Keep a LAN bridge endpoint on the trusted network and do not expose port 18423 to the Internet.

## WoW icons

The resolver expects a legally sourced 3.3.5a icon library under `public/assets/wow-icons`, with lowercase WebP filenames. Missing icons use the packaged neutral fallback.
