# Releasing WoW Companion Screen

Client and PWA releases are coordinated and use the same semantic version.

1. Create a focused branch from an up-to-date `main`; never implement directly on `main`.
2. Update the add-on manifest, bridge constant, PWA package and lockfile, README, and changelog to the release version.
3. Run `./scripts/validate-version.ps1`, the native build and CTest suite, and the PWA tests, type-check, and static build verification.
4. Open a pull request to `main`. Merge only after both CI workflows pass.
5. From the merged commit on `main`, create signed or annotated tags named `client-vX.Y.Z` and `pwa-vX.Y.Z`, then push both tags.
6. Confirm that GitHub Actions publishes both GitHub releases and that the PWA workflow publishes the matching container image tag.

The workflows reject malformed tags, tags not contained in `main`, and versions that disagree with committed release metadata.
