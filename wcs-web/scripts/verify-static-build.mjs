import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import sharp from "sharp";

const requestedBasePath = process.argv[2] ?? "";
const basePath = requestedBasePath && requestedBasePath !== "/"
  ? `/${requestedBasePath.replace(/^\/+|\/+$/g, "")}`
  : "";
const output = path.resolve("out");
const read = (file) => fs.readFileSync(path.join(output, file), "utf8");
const icons = [
  { src: "icons/favicon-32.png", size: 32, metadata: true },
  { src: "icons/apple-touch-icon-180.png", size: 180, metadata: true },
  { src: "icons/wcs-192.png", size: 192, purpose: "any" },
  { src: "icons/wcs-512.png", size: 512, purpose: "any" },
  { src: "icons/wcs-maskable-512.png", size: 512, purpose: "maskable" },
];

for (const file of ["index.html", "manifest.webmanifest", "sw.js", "icons/action-fallback.svg", "assets/wow-icons/spell_fire_flamebolt.webp", ...icons.map(({ src }) => src)]) {
  assert.ok(fs.existsSync(path.join(output, file)), `Static export is missing ${file}`);
}

for (const icon of icons) {
  const metadata = await sharp(path.join(output, icon.src)).metadata();
  assert.equal(metadata.format, "png", `${icon.src} is not a PNG`);
  assert.equal(metadata.width, icon.size, `${icon.src} has the wrong width`);
  assert.equal(metadata.height, icon.size, `${icon.src} has the wrong height`);
}

const html = read("index.html");
assert.match(html, new RegExp(`${basePath.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}/_next/`), "Next.js assets do not use the expected base path");
assert.ok(html.includes(`${basePath}/manifest.webmanifest`), "Manifest metadata does not use the expected base path");
for (const icon of icons.filter(({ metadata }) => metadata)) {
  assert.ok(html.includes(`${basePath}/${icon.src}`), `${icon.src} metadata does not use the expected base path`);
}

const manifest = JSON.parse(read("manifest.webmanifest"));
assert.equal(manifest.start_url, ".");
assert.equal(manifest.scope, ".");
assert.deepEqual(
  manifest.icons,
  icons.filter(({ purpose }) => purpose).map(({ src, size, purpose }) => ({
    src,
    sizes: `${size}x${size}`,
    type: "image/png",
    purpose,
  })),
);

const serviceWorker = read("sw.js");
assert.ok(serviceWorker.includes("self.registration.scope"), "Service worker cache paths are not scope-aware");
assert.ok(!serviceWorker.includes('caches.match("/")'), "Service worker contains a root-only offline fallback");
for (const icon of icons) {
  assert.ok(serviceWorker.includes(`"${icon.src}"`), `${icon.src} is missing from the service worker shell cache`);
}

console.log(`Verified static export for base path '${basePath || "/"}'.`);
