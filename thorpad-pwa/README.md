# ThorPad PWA

Static Next.js companion UI for the WarcraftXL Thor bridge.

```powershell
npm install
npm run dev
```

For UI work without a running WoW client, open `http://localhost:3000/?demo`. Demo mode skips the bridge connection and fills the PWA with representative player and action data in both development and static builds.

Production output is generated in `out/` by `npm run build`. Build the included Dockerfile for a lightweight Nginx image.

The browser connects directly to `ws://<WoW-PC-IP>:18423/thor`. Keep that bridge endpoint on the trusted LAN and do not expose port 18423 to the Internet.

## WoW icons

The resolver expects a legally sourced 3.3.5a icon library under `public/assets/wow-icons`, with lowercase WebP filenames. Missing icons use the packaged neutral fallback.
