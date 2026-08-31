# Releasing WoW Companion Screen

Native and PWA components share one semantic version. Merging a new validated version to `main` deploys the PWA to Pages and automatically publishes the client.

1. Create a focused branch from current `main`.
2. Update the add-on manifest, native component constants, PWA package and lockfile, and changelog.
3. Run `./scripts/validate-version.ps1`, native build/tests, and PWA tests, type-check, and Pages static-build verification.
4. Merge a pull request after both CI workflows pass.
5. Confirm Pages deploys and the client workflow creates `client-vX.Y.Z` plus a GitHub Release containing the ZIP and checksum.

The client workflow releases only when the matching tag is absent. Manually pushing a `client-vX.Y.Z` tag is a recovery path; it must point to `main` and match committed component metadata. Do not create PWA release tags, archives, or published container images.
