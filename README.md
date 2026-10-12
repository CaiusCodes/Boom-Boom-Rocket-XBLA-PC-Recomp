# Boom Boom Rocket XBLA PC Recomp

A native Windows PC recompilation of **Boom Boom Rocket**, the Xbox Live Arcade
rhythm game, using [ReXGlue](https://github.com/rexglue/rexglue-sdk). The original
Xbox 360 program is statically recompiled; this is not a wrapper that launches
Xenia. Current version: **0.9.1**.

> **You need your own legally obtained copy of Boom Boom Rocket.** This project
> contains no original game executable, graphics, sound, music or other game
> data. The launcher imports the supported Xbox 360 game package you provide, and the
> optional Boom Boom Rock Pack DLC, into your own portable game folder.

If you enjoy this project, you can support future recomp projects on Ko-fi.
Support is optional, and all projects and releases remain free.

[![Support on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/caiuscodes)

![Boom Boom Rocket title screen, showing version v0.9.1](docs/screenshots/title-screen.png)

## Requirements

- Windows 10 or 11, 64-bit
- An AVX2-capable x86-64-v3 CPU and a Direct3D 12-capable GPU
- .NET Framework 4.8 for the portable launcher
- Your own Boom Boom Rocket XBLA package (Title ID `5841086A`)
- Optional: your own **Boom Boom Rock Pack DLC** package

Keyboard and mouse work for player 1. Experimental local multiplayer requires
two Xbox-compatible controllers connected before launch.

## Install and play

1. Download `Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1.zip` from
   [Releases](https://github.com/CaiusCodes/Boom-Boom-Rocket-XBLA-PC-Recomp/releases).
2. Extract the whole ZIP into a writable folder, outside Program Files.
3. Open **Boom Boom Rocket.exe** in the extracted root. Select your original
   package, or drag and drop a package or its folder into the window.
   Packages can be extensionless; do not select a ZIP or `default.xex`.
4. Import runs immediately after selection. Add optional DLC if wanted, then
   click **Play**. First setup stays open until you choose to play. Optional
   Extras can start in fullscreen or create a desktop shortcut; both default off.
   One compatible package beside the launcher is imported automatically. If
   several are found, choose one from the list. No whole-computer scan is done.

The release root contains `Boom Boom Rocket.exe`, `resources/`, `README.txt`,
and `licenses/`. The native runtime is internal to `resources/`; only the root
launcher is user-facing. Settings, saves, assets, DLC and caches stay under
`resources/`. Move or back up the whole portable folder together.
No administrator access or registry installation is needed.

Subsequent launches start the game directly, with no import interface or original
package required. The original package is never modified or deleted.

For DLC on first launch, use **Browse DLC (optional)** before or after choosing
the game package, then click **Play**. **Skip DLC** clears a queued optional
selection. Alternatively, use **Download Content** on the main menu later.
Selecting it closes the game first and uses the same launcher. If installed,
the side description says **Boom Boom Rock Pack DLC is already installed**. To repair/reselect the base package, close the
game and run `"Boom Boom Rocket.exe" --manage`. DLC never blocks the initial import.

Browse Package is disabled after a successful first import. Fullscreen is saved
in the portable settings only if selected. Desktop shortcuts target the root
launcher and are optional; recreate the shortcut if you move the portable folder.
Existing shortcuts are never overwritten.

![Portable import window](docs/screenshots/setup.png)

For an older Setup/Game release, extract the new ZIP into the same portable root
to automatically copy compatible assets, settings and saves from `Game/resources`.
Alternatively drag the old portable folder into the first-launch window.
Old folders are left untouched as a backup; conflicting user-data destinations
are rejected rather than silently overwritten. Updates to the new layout reuse
assets and preserve saves; `resources/defaults.toml` never replaces your config.
Do not redistribute imported `resources/assets` or original game packages.

## Features

- Native Windows build using ReXGlue's graphics, audio and Xbox-service runtime
- Single-player modes and experimental local two-player Battle and Endurance
- Keyboard/mouse and Xbox-compatible controller support
- Native menu mouse selection; paced mouse-wheel navigation on song lists
- Windowed/fullscreen mode, resolution, VSync and optional FPS display
- Separate Gamepad Controls, Keyboard Controls and manual Timing Calibration pages
- Portable saves and optional DLC imported from your own files
- Automatic controller/keyboard rocket labels, unless you save a manual choice
- Title-screen version label and distinct setup/game icons

![Main menu](docs/screenshots/main-menu.png)

![Single-player gameplay with arrow rocket labels](docs/screenshots/gameplay.png)

![Settings with display mode, resolution, VSync, FPS and reset display options](docs/screenshots/settings.png)

## Controls

| Key / button | Action |
|---|---|
| WASD or arrow keys | Directions / menu navigation |
| Enter | Start / pause |
| Space | Bonus Run during gameplay (right trigger); Start in menus |
| Escape | Pause during play; B / back in menus |
| E or left click | A / confirm |
| B, Backspace or right click | B / back |
| X, Y | X and Y buttons |
| Mouse wheel on song lists | Move up/down; left click confirms the highlighted song |

Left click cycles Settings values and wraps through available choices. E/A
saves Settings; right click or Escape goes back. Without a controller the title
screen says *Press Any Key*, and controller-button footer hints are hidden.
Keyboard/mouse supplement player 1 only; local multiplayer needs two controllers.
See the included release `README.txt` for calibration and troubleshooting details.

## Known limitations

- Windows x64 only; local multiplayer is experimental. Real-controller hot-plug,
  timing feel and long two-player sessions still need wider user testing.
- Xbox Live, online multiplayer, system leaderboards, achievement UI and
  Marketplace screens are unavailable.
- The runtime uses one tested executable layout. The launcher verifies the
  XEX SHA256, not the container filename or hash:
  `B1BC45589CEC79E375E23FC48C122FA5902A2DFD6E3C9C765064DCD9E1A8709B`.
  Other executable revisions need separate compatibility testing.
- The launcher and the game are not digitally signed. Check the release's origin if
  Windows displays a warning. VSync defaults on; saved choices are preserved.

For bug reports, include the newest log from `resources/logs/`, after
checking it for personal installation paths. Do not attach original game files.

## For developers

You need Git, CMake 3.25+, Ninja, Clang 18+, Windows PowerShell, and the Visual
Studio C++ Build Tools with Windows SDK, plus your own game package. Tested with
Clang 22.1.8, CMake 4.4.2, Ninja 1.13.2 and .NET Framework 4.8.

Latest local verification warning: fresh ReXGlue code generation exited
unexpectedly. The updated deterministic hooks passed replay against previously
verified raw generator output, but a new full generation was not confirmed on
this machine. The runtime build and 89 launcher/import checks passed.

From the project directory:

```powershell
./tools/Get-Sdk.ps1
./tools/Build-Sdk.ps1
./tools/Regenerate.ps1 -GamePackage "PATH_TO_YOUR_ORIGINAL_PACKAGE"
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release --target boom_boom_rocket
./tools/Build-Launcher.ps1
```

ReXGlue is fetched into ignored `out/dependencies/rexglue-sdk`, pinned to commit
`3eb9b511b4140d2769e27be63eae57d41bfa2afa` (upstream 0.9.0). The fetch script
verifies required dependency submodule commits. Project changes are deterministic
patches in `patches/sdk.json`; no modified local SDK or root submodule is required.
The SDK installs to `out/sdk-install`.

`Regenerate.ps1` verifies the XEX, generates in private staging and applies
`patches/generated.json`. All 16 generated files are hash-verified; translated
game source is ignored and never edited by hand. The CMake codegen target uses
the same pipeline. Set `BBR_GAME_DIRECTORY` for extracted private input, and do
not run raw codegen over patched output. See [patches/README.md](patches/README.md).

For isolated builds, use new `-BuildDirectory`/`-InstallDirectory` arguments to
`Build-Sdk.ps1`, a new CMake `-B` directory with `-DBBR_SDK_PREFIX=...`, and the
matching regeneration `-SdkPrefix`. The game builds to
`out/build/win-amd64-release/boom_boom_rocket.exe`.

`Make Release.bat` builds and packages the single launcher. Alternatively use
`tools/Package-Launcher.ps1 -PortableFolder ...` and
`tools/Package-DependencySource.ps1 -Destination ...`. Publish the portable ZIP
and matching dependency-source ZIP together. GitHub's tagged source archive
contains the complete project; a separate project-source ZIP is not needed.
`VERSION` records the release version. Never commit private imports or `out/`.

Launcher/import details and test coverage: [docs/launcher.md](docs/launcher.md).
Release ZIP and immutable-file SHA-256 checksums are supplied. Executables are
ordinary unsigned builds, without an embedded runtime archive, packer or admin
request. This reduces unnecessary packaging complexity but cannot guarantee an
antivirus verdict. Do not disable Windows security to run the project.

## Licence and credits

Original project/tooling/integration code is MIT licensed; see [LICENSE](LICENSE).
Original or translated Boom Boom Rocket code and assets are **not** MIT licensed.
ReXGlue, Extract-STFS and other dependencies retain their own licences in
[packaging/licenses](packaging/licenses).

FFmpeg and libmspack are LGPL components in the runtime. The matching
`Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1-Dependency-Source.zip` accompanies the
portable ZIP and includes their source and runtime rebuild/relink instructions.
No original game files are needed to rebuild those libraries. Their licences
permit modifications and reverse engineering to debug those modifications.

Boom Boom Rocket, Xbox and Xbox 360 belong to their respective rights holders.
This is an unofficial project, not affiliated with or endorsed by them.
Screenshots are documentation, not installable game assets.
