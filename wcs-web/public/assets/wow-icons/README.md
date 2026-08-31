# WoW icon assets

Import icons from a legally owned WoW 3.3.5a client; this project does not redistribute Blizzard artwork.

From `wcs-web`:

```powershell
npm run icons:import -- "E:\WoW 3.3.5a"
```

The importer reads base and patch MPQs, converts the final `Interface\Icons` BLPs to lowercase WebP files, and skips outputs newer than their source. Use `--force` to rebuild or `--locale enUS` to select a locale. Exported `BlizzardInterfaceArt` or `Interface\Icons` directories are also accepted.

For example, `Interface\Icons\Spell_Fire_FlameBolt` becomes `spell_fire_flamebolt.webp`. Missing assets use the neutral fallback.
