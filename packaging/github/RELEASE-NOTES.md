# Boom Boom Rocket XBLA PC Recomp v0.9.1

## Publication preparation

- All four generated-code hook locations now apply through deterministic,
  hash-verified patches; clean regeneration reproduces the working source.
- Pinned upstream ReXGlue with 20 focused file patches replaces reliance on the
  shared modified SDK. Unrelated SDK changes and debug traces are excluded.
- Original project integration/tooling is MIT licensed. Third-party notices and
  matching LGPL dependency sources accompany the portable tool.
- No original game package, XEX, assets or generated game source is distributed.

- Setup now identifies the tested XEX revision before committing imports or
  runtime updates. Title ID alone is not enough for the embedded, fixed-address
  recompilation. The fingerprint is also recorded in release-manifest.json.

- Public ZIP root contains only Setup Boom Boom Rocket.exe, README.txt and licenses/.
- Setup creates Game/Boom Boom Rocket.exe, Game/resources/ and Game/release-manifest.json.
- Internal native runtime, matching DLLs, settings, imported assets, DLC and saves
  now live together under Game/resources.
- The manifest records Game-relative paths and immutable runtime file hashes,
  not private game files, saves or mutable settings.
- Older layouts are rejected with fresh-folder guidance; existing installations
  are not partially migrated or deleted.
- Both current and historical packaging entry points use the same allowlisted
  setup pipeline. No original XBLA package, XEX or development project is shipped
  in the portable ZIP; matching library sources are a separate release asset.

## Preserved menu polish

- Setup package browsers start beside the setup executable.
- Runtime F3/F4/F7/backtick shortcuts are disabled, with an opt-in diagnostic switch.
- Native Gamepad Controls, Keyboard Controls and manual Timing Calibration pages.
- Settings has nine rows, including Reset Display Settings; no player-name rows.
- Fresh installations pick two different random names from the supplied list;
  existing names and saves survive updates and DLC installation.
- Native menu mouse selection; song lists use paced wheel scrolling instead of
  hover selection, and left-click confirms the highlighted song.
- Hide controller-button icons in menus when no controller is connected.
- Timing adjustment shifts native note judgement, from -200 to +200 ms in 10 ms
  steps, applied at the next song/restart. It does not change music speed.
- Installer integration now also checks manifest hashes, clean root contents,
  the resources layout and legacy-folder protection; layout tests cover 100–200%.
- Timing feel, physical controller hot-plug and long two-player matches still
  need user testing. This remains a testing release, not a stability guarantee.

## Preserved changes from the local multiplayer baseline

- Controls revision: arrow keys supplement WASD. Esc sends Start during gameplay/countdown,
  and B in menus (including pause). Its key-down mapping is latched through screen transitions
  so releasing Esc after pause opens cannot leave Start stuck or send an extra Back.

- Experimental local Battle and Endurance through the game's original two-player menus.
- Controller 2 receives an offline `Player 2` profile with a stable, distinct ID.
- Player-two save data is isolated under `Game/resources/userdata/5841086A/profile/Player2`.
- Guest profile loads/writes support asynchronous completion and portable persistence.
- Main-menu help now asks for two controllers; all existing menu and portability changes remain.
- Verified both modes reach two-player gameplay, player-two pause navigation, and independent input indices
  using a private simulated second pad. That diagnostic driver is NOT included in the public runtime.
- Two real controllers, hot-plug recovery, and long matches still require user validation.
- Connect both controllers before launch. Keyboard/mouse supplement player 1 only.
- No Xbox Live accounts, online play, profile picker, or independent player-two achievement store added.

- Version numbering reset to `v0.9.1`; smoother title-screen version text at the same size.
- Player-one keyboard/mouse support, alongside controllers: WASD directions,
  Enter/Space Start, E/left-click A, B/Backspace/right-click B, X and Y.
- Focus loss and overlay capture release input; overlapping aliases work correctly.
- Skip the new-profile welcome acknowledgement while preserving creation and saving.
- Earlier baseline marked Multiplayer "coming soon"; this test build supersedes that text.

- Separate neon rocket icon for the play launcher and game window; setup retains its firework.
- Small version label at the bottom left of the title screen only.
- Existing native menu changes, portable data layout and VSync default preserved.
- Normalize ZIP path separators before using extended Windows paths (PowerShell 7 builds).

- Fixed high-DPI startup layout: scale controls only after construction finishes.
- Text, path frames, section numbers and buttons now use the same scaled layout.
- Fixed double DPI scaling of the drawn SETUP subtitle.
- VSync remains on by default. Existing saved settings are preserved.
- Added simulated startup layout checks at 100%, 125%, 150%, 175% and 200%.

- One setup EXE with the Minimal Neon design.
- Base-game selection and optional Boom Boom Rock Pack DLC on the same screen.
- Renamed installer: `Setup Boom Boom Rocket.exe`.
- Download Content help now directs players to `Setup Boom Boom Rocket.exe`.
- Product and window title are now `Boom Boom Rocket`.
- Main menu uses `Exit Game`, with help text `Quit Boom Boom Rocket and return to desktop.`
- Extracts `Game/Boom Boom Rocket.exe`, with an original neon rocket icon.
- Runtime, DLLs, assets, DLC, settings, logs and saves are contained in `Game/resources`.
- Reopen setup in the portable folder to add DLC later.
- Existing saves, achievements and configuration are preserved.
- Package validation, progress, cancellation and rollback.
- No game or DLC assets included.

Extract the ZIP to a fresh writable folder and run **Setup Boom Boom Rocket.exe**. You will need your
own original Xbox 360 game package and, optionally, the DLC package.

Earlier installer engine and launcher baseline: 30 checks passed, including relocation and an
unrelated starting working directory. Imports previously matched 320 unchanged
base-game files and 80 DLC files from the working test installation. The native
setup interface has been visually checked. A new-machine gameplay test remains
recommended for this release.

This release enables local multiplayer experimentally. The tagged source archive
covers the complete integration project, setup, tooling, tests and deterministic
patches. Rebuilding the game runtime requires the user's original game package
and the pinned dependencies fetched by the documented scripts. Publish the
matching `Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1-Dependency-Source.zip` alongside
`Boom-Boom-Rocket-XBLA-PC-Recomp-v0.9.1.zip`; a separate project-source ZIP is not
needed on GitHub.
