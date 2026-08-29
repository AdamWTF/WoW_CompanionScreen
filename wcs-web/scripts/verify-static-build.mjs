import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";

const requestedBasePath = process.argv[2] ?? "";
const basePath = requestedBasePath && requestedBasePath !== "/"
  ? `/${requestedBasePath.replace(/^\/+|\/+$/g, "")}`
  : "";
const output = path.resolve("out");
const read = (file) => fs.readFileSync(path.join(output, file), "utf8");

for (const file of ["index.html", "manifest.webmanifest", "sw.js", "icons/wcs.svg", "icons/action-fallback.svg", "assets/wow-icons/spell_fire_flamebolt.webp"]) {
  assert.ok(fs.existsSync(path.join(output, file)), `Static export is missing ${file}`);
}

const html = read("index.html");
assert.match(html, new RegExp(`${basePath.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}/_next/`), "Next.js assets do not use the expected base path");
assert.ok(html.includes(`${basePath}/manifest.webmanifest`), "Manifest metadata does not use the expected base path");
assert.ok(html.includes(`${basePath}/icons/wcs.svg`), "Application icon metadata does not use the expected base path");

const manifest = JSON.parse(read("manifest.webmanifest"));
assert.equal(manifest.start_url, ".");
assert.equal(manifest.scope, ".");
assert.equal(manifest.icons[0].src, "icons/wcs.svg");

const serviceWorker = read("sw.js");
assert.ok(serviceWorker.includes("self.registration.scope"), "Service worker cache paths are not scope-aware");
assert.ok(!serviceWorker.includes('caches.match("/")'), "Service worker contains a root-only offline fallback");

console.log(`Verified static export for base path '${basePath || "/"}'.`);
