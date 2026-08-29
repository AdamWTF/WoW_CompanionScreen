# WoW Companion Screen PWA

This is the touch-friendly companion app used by WoW Companion Screen.

## Run it locally

```powershell
npm install
npm run dev
```

Open `http://localhost:3000/?demo` to work on the interface without running WoW. Demo mode uses representative character and action data and does not try to connect to the bridge.

Run `npm run build` to create a production build in `out/`. The included Dockerfile serves that build through Nginx.

## Hosted app

The public app lives at:

```text
https://adamwtf.github.io/WoW_CompanionScreen/
```

Add `?demo` to open it with demo data.

The hosted app works live when it runs on the same Windows device as WoW, including the AYN Thor's second screen. Install it from the browser and set **WoW PC IPv4 address** to `127.0.0.1`.

GitHub Pages serves the app files, but the live connection goes straight back to WoW through:

```text
ws://127.0.0.1:18423/wcs
```

Game state and controls are not sent through GitHub.

GitHub Pages builds use `/WoW_CompanionScreen` as their base path. Static release and Docker builds use `/`.

## Use it from another device

An HTTPS page generally can't open a plaintext `ws://` connection to another machine on your LAN. For a phone, tablet or separate computer, serve the PWA over HTTP on your trusted local network.

You can run the published container with:

```powershell
docker run --rm -p 8080:80 ghcr.io/adamwtf/wow-companion-screen-pwa:latest
```

Open `http://<PWA-host-IP>:8080` on the companion device, then enter the local IPv4 address of the PC running WoW.

The browser connects to `ws://<WoW-PC-IP>:18423/wcs`. Keep that port on your trusted network and never expose it to the Internet.

Live control works over HTTP. Installation and offline PWA features may not, because browsers normally require HTTPS or localhost for those features.

## WoW icons

The app expects a legally sourced 3.3.5a icon library in `public/assets/wow-icons`, using lowercase WebP filenames.

If an icon is missing, the app uses its neutral fallback instead.
