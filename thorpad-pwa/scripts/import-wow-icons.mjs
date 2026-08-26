import { closeSync, existsSync, openSync, readFileSync, readSync, statSync } from "node:fs";
import { mkdir, readdir, rename, stat, unlink, writeFile } from "node:fs/promises";
import { createRequire } from "node:module";
import path from "node:path";
import { pathToFileURL } from "node:url";

import sharp from "sharp";
import { blpToPng, initSync } from "wow-blp-web";
import { initSync as initMpq, MpqStreamingArchive } from "wow-mpq-web";

const require = createRequire(import.meta.url);
const packageEntry = require.resolve("wow-blp-web");
const wasmPath = path.join(path.dirname(packageEntry), "wow_blp_web_bg.wasm");
initSync({ module: readFileSync(wasmPath) });
const mpqPackageEntry = require.resolve("wow-mpq-web");
const mpqWasmPath = path.join(path.dirname(mpqPackageEntry), "wow_mpq_web_bg.wasm");
initMpq({ module: readFileSync(mpqWasmPath) });

const DEFAULT_OUTPUT = path.resolve("public", "assets", "wow-icons");
const ICON_SUBDIRECTORIES = [
  ["Interface", "Icons"],
  ["Interface", "ICONS"],
  ["BlizzardInterfaceArt", "Interface", "Icons"],
  ["BlizzardInterfaceArt", "Interface", "ICONS"],
];

export function iconOutputName(sourcePath) {
  return `${path.basename(sourcePath, path.extname(sourcePath)).toLowerCase()}.webp`;
}

export function findIconDirectory(inputPath) {
  const absolute = path.resolve(inputPath);
  if (!existsSync(absolute)) throw new Error(`Source path does not exist: ${absolute}`);

  for (const parts of ICON_SUBDIRECTORIES) {
    const candidate = path.join(absolute, ...parts);
    if (existsSync(candidate)) return candidate;
  }

  return absolute;
}

function isWowInstallation(directory) {
  return existsSync(path.join(directory, "Wow.exe")) && existsSync(path.join(directory, "Data"));
}

async function findBlpFiles(directory) {
  const files = [];
  for (const entry of await readdir(directory, { withFileTypes: true })) {
    const entryPath = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...(await findBlpFiles(entryPath)));
    else if (entry.isFile() && path.extname(entry.name).toLowerCase() === ".blp") files.push(entryPath);
  }
  return files;
}

async function destinationIsCurrent(sourceModifiedMs, destination) {
  if (!existsSync(destination)) return false;
  const destinationInfo = await stat(destination);
  return destinationInfo.mtimeMs >= sourceModifiedMs;
}

async function convertIconBytes(bytes, destination, sourceModifiedMs, force) {
  if (!force && (await destinationIsCurrent(sourceModifiedMs, destination))) return "skipped";
  const png = blpToPng(bytes);
  const webp = await sharp(png).webp({ quality: 90, effort: 4 }).toBuffer();
  const temporary = `${destination}.${process.pid}.tmp`;
  await writeFile(temporary, webp);
  try {
    await rename(temporary, destination);
  } catch (error) {
    await unlink(temporary).catch(() => {});
    throw error;
  }
  return "converted";
}

async function convertIconFile(source, destination, force) {
  const sourceInfo = await stat(source);
  return convertIconBytes(new Uint8Array(readFileSync(source)), destination, sourceInfo.mtimeMs, force);
}

function parseArguments(argv) {
  const positional = [];
  let output = DEFAULT_OUTPUT;
  let force = false;
  let locale;

  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index];
    if (argument === "--force") force = true;
    else if (argument === "--locale") {
      locale = argv[++index];
      if (!locale) throw new Error("--locale requires a locale such as enUS.");
    }
    else if (argument === "--output") {
      output = path.resolve(argv[++index] ?? "");
      if (!argv[index]) throw new Error("--output requires a directory.");
    } else positional.push(argument);
  }

  if (positional.length !== 1) {
    throw new Error('Usage: npm run icons:import -- "C:\\path\\to\\WoW 3.3.5a" [--locale enUS] [--force]');
  }
  const input = path.resolve(positional[0]);
  return { source: isWowInstallation(input) ? input : findIconDirectory(input), output, force, locale };
}

export async function importWowIcons({ source, output = DEFAULT_OUTPUT, force = false }) {
  const files = await findBlpFiles(source);
  if (files.length === 0) throw new Error(`No .blp files found beneath ${source}`);

  const names = new Map();
  for (const file of files) {
    const outputName = iconOutputName(file);
    const prior = names.get(outputName);
    if (prior) throw new Error(`Duplicate icon filename ${outputName}:\n  ${prior}\n  ${file}`);
    names.set(outputName, file);
  }

  await mkdir(output, { recursive: true });
  let converted = 0;
  let skipped = 0;
  const failures = [];
  const concurrency = Math.max(1, Math.min(8, files.length));
  let nextIndex = 0;

  async function worker() {
    while (nextIndex < files.length) {
      const index = nextIndex++;
      const sourceFile = files[index];
      const destination = path.join(output, iconOutputName(sourceFile));
      try {
        const result = await convertIconFile(sourceFile, destination, force);
        if (result === "converted") converted += 1;
        else skipped += 1;
      } catch (error) {
        failures.push(`${sourceFile}: ${error instanceof Error ? error.message : String(error)}`);
      }

      const completed = converted + skipped + failures.length;
      if (completed % 250 === 0 || completed === files.length) {
        console.log(`Processed ${completed}/${files.length} icons...`);
      }
    }
  }

  await Promise.all(Array.from({ length: concurrency }, () => worker()));
  if (failures.length > 0) {
    const preview = failures.slice(0, 10).join("\n");
    throw new Error(`Failed to convert ${failures.length} icon(s):\n${preview}`);
  }
  return { found: files.length, converted, skipped, output };
}

function archivePriority(name) {
  const lower = name.toLowerCase();
  if (lower.startsWith("base-")) return 10;
  if (lower.startsWith("locale-")) return 20;
  if (lower === "common.mpq") return 20;
  if (lower === "common-2.mpq") return 30;
  if (lower.startsWith("expansion-locale-") || lower === "expansion.mpq") return 40;
  if (lower.startsWith("lichking-locale-") || lower === "lichking.mpq") return 50;
  if (/^patch(?:-[a-z]{2}[a-z]{2})?\.mpq$/i.test(name)) return 60;
  const patchNumber = lower.match(/^patch(?:-[a-z]{2}[a-z]{2})?-(\d+)\.mpq$/)?.[1];
  return patchNumber ? 60 + Number(patchNumber) : 0;
}

async function findMpqArchives(wowRoot, requestedLocale) {
  const dataDirectory = path.join(wowRoot, "Data");
  const topLevel = (await readdir(dataDirectory, { withFileTypes: true }))
    .filter((entry) => entry.isFile() && entry.name.toLowerCase().endsWith(".mpq") && archivePriority(entry.name) > 0)
    .map((entry) => path.join(dataDirectory, entry.name));
  const locales = (await readdir(dataDirectory, { withFileTypes: true }))
    .filter((entry) => entry.isDirectory() && /^[a-z]{2}[A-Z]{2}$/.test(entry.name))
    .map((entry) => entry.name);
  const locale = requestedLocale ?? (locales.length === 1 ? locales[0] : undefined);
  if (!locale) throw new Error(`Multiple WoW locales found (${locales.join(", ")}); pass --locale followed by one of them.`);
  if (!locales.includes(locale)) throw new Error(`Locale ${locale} was not found beneath ${dataDirectory}.`);

  const localeDirectory = path.join(dataDirectory, locale);
  const localized = (await readdir(localeDirectory, { withFileTypes: true }))
    .filter((entry) => entry.isFile() && entry.name.toLowerCase().endsWith(".mpq") && archivePriority(entry.name) > 0)
    .map((entry) => path.join(localeDirectory, entry.name));
  return [...topLevel, ...localized].sort((left, right) => archivePriority(path.basename(left)) - archivePriority(path.basename(right)));
}

function withStreamingArchive(archivePath, callback) {
  const descriptor = openSync(archivePath, "r");
  const file = { descriptor, size: statSync(archivePath).size };
  const readRange = (handle, offset, length) => {
    const buffer = Buffer.allocUnsafe(length);
    const bytesRead = readSync(handle.descriptor, buffer, 0, length, offset);
    return new Uint8Array(buffer.buffer, buffer.byteOffset, bytesRead);
  };

  try {
    const archive = new MpqStreamingArchive(file, readRange);
    try {
      return callback(archive);
    } finally {
      archive.free();
    }
  } finally {
    closeSync(descriptor);
  }
}

export async function importWowIconsFromMpqs({ wowRoot, output = DEFAULT_OUTPUT, force = false, locale }) {
  const archives = await findMpqArchives(wowRoot, locale);
  const selectedIcons = new Map();

  for (const archivePath of archives) {
    const paths = withStreamingArchive(archivePath, (archive) => archive.listfile() ?? []);
    let count = 0;
    for (const internalPath of paths) {
      if (!/^interface[\\/]icons[\\/].+\.blp$/i.test(internalPath)) continue;
      selectedIcons.set(iconOutputName(internalPath), { archivePath, internalPath });
      count += 1;
    }
    if (count > 0) console.log(`${path.basename(archivePath)}: ${count} icon entries`);
  }

  if (selectedIcons.size === 0) throw new Error(`No Interface\\Icons BLP files were listed by the MPQs beneath ${wowRoot}`);
  await mkdir(output, { recursive: true });
  const iconsByArchive = new Map();
  for (const [outputName, icon] of selectedIcons) {
    const group = iconsByArchive.get(icon.archivePath) ?? [];
    group.push({ ...icon, outputName });
    iconsByArchive.set(icon.archivePath, group);
  }

  let converted = 0;
  let skipped = 0;
  const failures = [];
  let completed = 0;
  for (const [archivePath, icons] of iconsByArchive) {
    const archiveModifiedMs = statSync(archivePath).mtimeMs;
    withStreamingArchive(archivePath, (archive) => {
      for (const icon of icons) {
        try {
          icon.bytes = archive.readFile(icon.internalPath);
        } catch (error) {
          failures.push(`${archivePath} :: ${icon.internalPath}: ${error instanceof Error ? error.message : String(error)}`);
        }
      }
    });

    let nextIcon = 0;
    async function worker() {
      while (nextIcon < icons.length) {
        const icon = icons[nextIcon++];
        if (!icon.bytes) continue;
        try {
          const result = await convertIconBytes(icon.bytes, path.join(output, icon.outputName), archiveModifiedMs, force);
          if (result === "converted") converted += 1;
          else skipped += 1;
        } catch (error) {
          failures.push(`${archivePath} :: ${icon.internalPath}: ${error instanceof Error ? error.message : String(error)}`);
        }
        icon.bytes = undefined;
        completed += 1;
        if (completed % 250 === 0 || completed === selectedIcons.size) {
          console.log(`Processed ${completed}/${selectedIcons.size} icons...`);
        }
      }
    }
    await Promise.all(Array.from({ length: Math.min(8, icons.length) }, () => worker()));
  }

  if (failures.length > 0) {
    const preview = failures.slice(0, 10).join("\n");
    console.warn(`Skipped ${failures.length} invalid or unsupported MPQ icon entr${failures.length === 1 ? "y" : "ies"}:\n${preview}`);
  }
  return { found: selectedIcons.size, converted, skipped, failed: failures.length, output };
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  console.log(`Importing WoW icons from ${options.source}`);
  const result = isWowInstallation(options.source)
    ? await importWowIconsFromMpqs({ wowRoot: options.source, output: options.output, force: options.force, locale: options.locale })
    : await importWowIcons(options);
  const failed = result.failed ? `, ${result.failed} invalid archive entries skipped` : "";
  console.log(`Done: ${result.converted} converted, ${result.skipped} already current${failed} (${result.found} total).`);
  console.log(`PWA assets: ${result.output}`);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch((error) => {
    console.error(error instanceof Error ? error.message : error);
    process.exitCode = 1;
  });
}
