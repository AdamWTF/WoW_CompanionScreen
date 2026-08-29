# WoW Companion Screen PWA

Static Next.js UI for the WoW Companion Screen bridge.

```powershell
npm install
npm run dev
```

For UI work without a running WoW client, open `http://localhost:3000/?demo`. Demo mode skips the bridge connection and fills the PWA with representative player and action data in both development and static builds.

Production output is generated in `out/` by `npm run build`. Build the included Dockerfile for a lightweight Nginx image.

The hosted demo is deployed to `https://adamwtf.github.io/WoW_CompanionScreen/?demo`. GitHub Pages uses the `/WoW_CompanionScreen` build base path; root-hosted static and Docker builds continue to use `/`.

Because Pages is HTTPS and the native bridge currently exposes plaintext `ws://`, browsers block live bridge connections from the hosted site. Use the root-hosted release ZIP or the container on the trusted LAN for live control:

```powershell
docker run --rm -p 8080:80 ghcr.io/adamwtf/wow-companion-screen-pwa:latest
```

Open `http://<WoW-PC-IP>:8080` on the companion device. HTTP LAN hosting supports the live bridge, while service-worker installation and offline behavior require HTTPS or localhost.

The browser connects directly to `ws://<WoW-PC-IP>:18423/wcs`. Keep that bridge endpoint on the trusted LAN and do not expose port 18423 to the Internet.

## WoW icons

The resolver expects a legally sourced 3.3.5a icon library under `public/assets/wow-icons`, with lowercase WebP filenames. Missing icons use the packaged neutral fallback.
