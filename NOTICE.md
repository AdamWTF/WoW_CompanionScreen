# Thanks and attribution

WoW Companion Screen would not have been possible without
[WarcraftXL](https://github.com/WarcraftXL/wxl-core). We are deeply grateful to the WarcraftXL
developers and contributors for sharing their work openly. Their native runtime, engine bindings,
offset catalog, extension ABI, asset support, patching infrastructure, documentation, and patient
reverse-engineering work gave this project the foundation on which it could grow. Thank you.

WoW Companion Screen has since developed in a different direction, but that does not diminish the
importance of the work it began from. We are proud to retain that history and to credit the people
whose work made this project feasible in the first place.

The repository retains the original Git history and per-file WarcraftXL copyright notices. The
`wxl::` namespaces, `WXL_*` API symbols, and `include/wxl` headers identify the inherited SDK boundary
and are intentionally preserved for provenance and compatibility within the codebase.

WoW Companion Screen's additions include controller support, controller UI navigation, Smart
Interact, the in-game companion add-on, the paired local WebSocket bridge, and the companion web
application.

WarcraftXL and WoW Companion Screen are licensed under the GNU General Public License v3.0. Third-party
libraries and assets retain their own notices and license files under `deps/` and alongside the
relevant components.
