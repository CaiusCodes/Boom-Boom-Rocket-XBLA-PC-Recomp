# Boom Boom Rocket XBLA PC Recomp v0.9.1

## Portable launcher update — 12 October 2026

The existing v0.9.1 tag and downloads have been refreshed for this build.

- One user-facing executable: **Boom Boom Rocket.exe**. No separate setup EXE.
- Select your legally obtained original Xbox 360 game package, or drag a package
  or its folder into the import window. A single compatible package beside the
  launcher is detected automatically; multiple matches require a choice.
- Import the optional **Boom Boom Rock Pack DLC** before or after the base game,
  then click **Play**. First setup stays open; later launches go straight into
  the game without requiring the original package again.
- Refined original-project neon interface, with wider buttons, improved spacing
  and no First Launch heading. Browse Package is disabled after successful import.
- Optional Extras: start in fullscreen and create a desktop shortcut. Both default
  off and apply on Play. Existing shortcuts are never overwritten.
- Main-menu **Download Content** opens the same launcher's DLC importer when the
  pack is absent. Its description reads **Select to install your Boom Boom Rock
  Pack DLC.** Once present, it reads **Boom Boom Rock Pack DLC is already installed.**
- No Manage Game Files entry in Help & Options. To repair/re-import, close the game
  and run `"Boom Boom Rocket.exe" --manage`.
- Interrupted imports use staging, locking and journaled rollback. Original
  packages are never modified or automatically deleted.
- Ordinary EXE/DLL packaging replaces the embedded executable archive. No packer,
  obfuscation, administrator privileges or system-wide installation.

## Install / update

Extract the complete portable ZIP into a writable folder and open
**Boom Boom Rocket.exe**. Users must supply their own legally obtained game
package and, optionally, the Rock Pack DLC. This project does not distribute the
original package, XEX, graphics, sound, music or other game data.

```text
Boom Boom Rocket/
├── Boom Boom Rocket.exe
├── resources/
├── README.txt
└── licenses/
```

The internal native runtime, imported assets, configuration, DLC and saves live
under `resources/`. Move or back up the complete folder together. A desktop
shortcut must be recreated if the portable folder is relocated.

For existing single-launcher copies, replace the supplied files, including the
runtime files under resources; retain imported assets, userdata and your saved
`boom_boom_rocket.toml`. Defaults do not overwrite saved settings.
Older Setup/Game copies can migrate compatible assets, settings and saves from
`Game/resources` when the new ZIP is extracted into the same portable root.
Old directories remain untouched; conflicting user-data destinations are rejected
rather than silently overwritten.

## Preserved features

- Native Windows recompilation using pinned ReXGlue, not an Xenia launcher.
- Keyboard, mouse and controller support; Space is Bonus Run during gameplay,
  Enter starts and Escape pauses/resumes during gameplay or goes back in menus.
- Native display settings, VSync on by default, optional FPS display and manual
  timing calibration. Automatic rocket labels retain user-saved preferences.
- Experimental local two-controller multiplayer, with separate offline profiles.
- Original menu improvements, including removal of unsupported achievement and
  leaderboard entries, first-play chooser bypass and clean desktop exit.
- MIT-licensed project integration/tooling, preserved third-party notices and
  matching dependency sources. MIT does not license the original game or assets.

## Validation and limitations

- Build succeeded and 89 launcher/import checks passed, including game-first DLC
  import, explicit Play gating, disabled Browse Package, opt-in fullscreen,
  shortcut target/collision protection, migration, cancellation and crash recovery.
- The game and DLC import/handoff were locally launched; wider new-machine,
  real-controller hot-plug and long multiplayer testing remain recommended.
- Packages are validated against the supported executable revision, not Title ID
  alone. Other executable revisions require compatibility testing.
- Executables remain unsigned. Antivirus false positives cannot be ruled out;
  do not disable Windows security.
- Developer warning: fresh ReXGlue code generation exited unexpectedly during
  the latest local check. Updated deterministic hooks passed replay against
  previously verified raw generator output; a new full generation was not
  confirmed on this machine. Generated translated game source is not committed.

## Downloads

- `Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1.zip` — portable tool, without game data.
- `Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1-Dependency-Source.zip` — matching SDK and
  library source/licences. This is for developers and licence compliance, not play.
- GitHub's tagged source archive contains the project integration, tooling and
  deterministic patches; no separate project Source.zip is needed.

Immutable supplied-file SHA-256 checksums are inside resources/SHA256SUMS.txt.

### Download SHA-256

```text
BBBC2DE86A3F4A36F67D8991A951067D5E669525E28AE8FFC1E03215DFAEBE34  Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1.zip
2E5BA81D8FE690A8C1F159F987819B68D3B276B132957F04606F8F6DDD23A194  Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1-Dependency-Source.zip
```
