import { readFile } from "node:fs/promises";
import { mkdtemp, mkdir, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";

import sharp from "sharp";
import { afterEach, describe, expect, it } from "vitest";
import { pngToBlp } from "wow-blp-web";
import { MpqArchive } from "wow-mpq-web";

import { findIconDirectory, iconOutputName, importWowIcons, importWowIconsFromMpqs } from "./import-wow-icons.mjs";

const temporaryDirectories = [];

afterEach(async () => {
  await Promise.all(temporaryDirectories.splice(0).map((directory) => rm(directory, { recursive: true, force: true })));
});

describe("WoW icon importer", () => {
  it("finds an exported art tree and creates the WebP name used by the PWA", async () => {
    const root = await mkdtemp(path.join(tmpdir(), "thorpad-icons-"));
    temporaryDirectories.push(root);
    const iconDirectory = path.join(root, "BlizzardInterfaceArt", "Interface", "Icons");
    const output = path.join(root, "output");
    await mkdir(iconDirectory, { recursive: true });

    const png = await sharp({
      create: { width: 64, height: 64, channels: 4, background: { r: 220, g: 80, b: 30, alpha: 1 } },
    }).png().toBuffer();
    const source = path.join(iconDirectory, "Spell_Test_Flame.BLP");
    await writeFile(source, pngToBlp(png, { format: "dxt5" }));

    expect(findIconDirectory(root)).toBe(iconDirectory);
    expect(iconOutputName(source)).toBe("spell_test_flame.webp");

    const result = await importWowIcons({ source: iconDirectory, output });
    const webp = await readFile(path.join(output, "spell_test_flame.webp"));
    const metadata = await sharp(webp).metadata();
    expect(result).toMatchObject({ found: 1, converted: 1, skipped: 0 });
    expect(metadata).toMatchObject({ format: "webp", width: 64, height: 64 });
  });

  it("extracts an icon directly from a WoW locale MPQ", async () => {
    const root = await mkdtemp(path.join(tmpdir(), "thorpad-mpq-icons-"));
    temporaryDirectories.push(root);
    const localeDirectory = path.join(root, "Data", "enUS");
    const output = path.join(root, "output");
    await mkdir(localeDirectory, { recursive: true });

    const png = await sharp({
      create: { width: 64, height: 64, channels: 4, background: { r: 20, g: 100, b: 210, alpha: 1 } },
    }).png().toBuffer();
    const archive = MpqArchive.create();
    archive.addFile("Interface\\Icons\\Ability_Test_Blue.blp", pngToBlp(png, { format: "dxt5" }));
    await writeFile(path.join(localeDirectory, "locale-enUS.MPQ"), archive.export());
    archive.free();

    const result = await importWowIconsFromMpqs({ wowRoot: root, output });
    const webp = await readFile(path.join(output, "ability_test_blue.webp"));
    expect(result).toMatchObject({ found: 1, converted: 1, skipped: 0, failed: 0 });
    expect(await sharp(webp).metadata()).toMatchObject({ format: "webp", width: 64, height: 64 });
  });
});
