# Releasing WoW Companion Screen

Native and PWA components share one semantic version. The client is released from a tag; the PWA deploys to GitHub Pages from `main`.

1. Create a focused branch from current `main`.
2. Update the add-on manifest, native component constants, PWA package and lockfile, and changelog.
3. Run `./scripts/validate-version.ps1`, native build/tests, and PWA tests, type-check, and Pages static-build verification.
4. Merge a pull request after both CI workflows pass. Confirm the Pages deployment succeeds.
5. From that merged commit, create and push a signed or annotated `client-vX.Y.Z` tag.
6. Confirm the client GitHub Release contains its ZIP and checksum.

The client workflow rejects malformed tags, tags outside `main`, and versions that disagree with committed component metadata. Do not create PWA release tags, archives, or published container images.
