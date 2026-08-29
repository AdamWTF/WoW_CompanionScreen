# WoW icon assets

WoW Companion Screen deliberately does not redistribute Blizzard's artwork. Import the icons directly from your own WoW 3.3.5a installation before building the PWA.

## Import directly from the MPQs

From `wcs-web`, point the importer at the directory containing `Wow.exe` and `Data`:

```powershell
npm run icons:import -- "E:\\WoW 3.3.5a"
```

The importer reads the base and patch MPQs in precedence order, selects the final version of each `Interface\\Icons` BLP, converts it to WebP, and writes the lowercase files expected by the PWA into this directory. It skips outputs newer than their source archive; pass `--force` to rebuild every icon. If the installation contains multiple locales, select one with `--locale enUS`.

An already-exported `BlizzardInterfaceArt` or `Interface\\Icons` directory is still accepted. After importing, run `npm run build` as usual.

The bridge path `Interface\\Icons\\Spell_Fire_FlameBolt` resolves to `spell_fire_flamebolt.webp`. WoW Companion Screen falls back to its packaged neutral action icon if a requested asset is unavailable.
