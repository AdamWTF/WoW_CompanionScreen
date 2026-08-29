import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import sharp from "sharp";

const projectRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const source = path.join(projectRoot, "assets", "branding", "wow-companion-screen.jpg");
const outputDirectory = path.join(projectRoot, "public", "icons");

const icons = [
  ["favicon-32.png", 32],
  ["apple-touch-icon-180.png", 180],
  ["wcs-192.png", 192],
  ["wcs-512.png", 512],
];

await fs.mkdir(outputDirectory, { recursive: true });

for (const [filename, size] of icons) {
  await sharp(source)
    .resize(size, size, { fit: "fill", kernel: sharp.kernel.lanczos3 })
    .png({ compressionLevel: 9 })
    .toFile(path.join(outputDirectory, filename));
}

const maskableArtworkSize = 400;
await sharp(source)
  .resize(maskableArtworkSize, maskableArtworkSize, {
    fit: "fill",
    kernel: sharp.kernel.lanczos3,
  })
  .extend({
    top: (512 - maskableArtworkSize) / 2,
    bottom: (512 - maskableArtworkSize) / 2,
    left: (512 - maskableArtworkSize) / 2,
    right: (512 - maskableArtworkSize) / 2,
    background: "#100d09",
  })
  .png({ compressionLevel: 9 })
  .toFile(path.join(outputDirectory, "wcs-maskable-512.png"));

console.log(`Generated ${icons.length + 1} brand icons from ${path.relative(projectRoot, source)}.`);
