# Repository rules

These instructions apply to the entire repository.

## Supported target

- Target only the 32-bit World of Warcraft 3.3.5a client, build 12340, unless the user explicitly changes scope.
- Native code is C++20 and must be configured with the Win32 generator.
- Preserve the WarcraftXL extension ABI, existing `wxl::` namespaces, exported entry points, Lua globals, protocol messages, action mappings, and persisted formats unless a requested change requires otherwise.

## Architecture boundaries

- `src/` owns the injected core, client hooks, patcher, and shared runtime.
- `extensions/` contains independently loaded extensions. Extensions use the SDK in `include/` and `src/game/`; they must not include `offsets/` directly.
- `addon/WoWCompanionScreen/` owns in-game configuration, secure action buttons, controller mappings, and second-screen slots.
- `wcs-web/` owns the static companion PWA. It communicates directly with `wcs-bridge` and must not add a cloud relay.
- Keep WoW calls on the game thread. Worker threads may collect input or network data but must queue game-facing work for the main update path.

## Change discipline

- Inspect the working tree first. Preserve unrelated user changes and never discard them to simplify a task.
- Keep changes scoped to the request. Do not refactor adjacent code without a concrete need.
- Ask before adding features, changing supported platforms, making broad behavior or architecture changes, or adding/restructuring documentation sections.
- Do not edit vendored code or notices under `deps/` unless explicitly requested.
- Do not commit generated files, build outputs, client binaries, logs, caches, `wcs-web/out`, or `wcs-web/node_modules`.
- Do not commit, push, create/delete releases, publish packages, or change external repository settings unless explicitly requested.

## Documentation

- Write for the named audience and task. Prefer short instructions, tables, or exact commands over narrative explanation.
- Do not duplicate material across the root README, user guides, and component contracts. Link to the authoritative document.
- Do not add background, rationale, compatibility claims, warnings, or new sections unless they help the reader complete the documented task.
- Keep safety warnings direct and state them once per standalone user document.
- Update documentation when a requested behavior, interface, setup step, or supported delivery method changes.

## Build and test

Run checks relevant to the changed subsystem. Report any check that cannot run and why.

Native configure, build, and tests:

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release --target wcs-core wcs-patcher wcs-gamepad wcs-bridge wcs-gamepad-tests wcs-gamepad-smart-interact-tests wcs-bridge-tests wcs-bridge-websocket-tests --parallel
ctest --test-dir build -C Release --output-on-failure
```

PWA checks, from `wcs-web`:

```powershell
npm test
npm run typecheck
npm run build
```

Use focused tests while iterating, then run the relevant complete set before finishing. Run `git diff --check` for every change.

## Delivery

- GitHub Pages from `main` is the only maintained PWA publication channel.
- Do not add PWA release archives, PWA release tags, or published container-image workflows.
- The retained Docker files are for unsupported source-based LAN self-hosting only.
- Native client releases remain tag-driven through `.github/workflows/client.yml`.
