BOOM BOOM ROCKET XBLA PC RECOMP - v0.9.1
=======================================

You need your own legally obtained original Boom Boom Rocket XBLA package.
No original Xbox executable, graphics, music, sound or game data is supplied.
Boom Boom Rock Pack DLC is optional and must also be supplied by you.

FIRST LAUNCH
------------
Extract the whole ZIP to a writable folder and open Boom Boom Rocket.exe.
Select your original package, or drag a package or its folder into the window.
Import starts immediately. Add optional DLC if wanted, then click Play.
First setup stays open until you choose to play. Packages can have no extension; do not select a ZIP or
default.xex. One supported package beside the launcher is detected automatically.
Multiple compatible packages are shown for you to choose. No whole-PC scan occurs.

Browse Package is greyed out after a successful import. Optional Extras:
- Start in fullscreen: saves fullscreen in the portable game settings.
- Create desktop shortcut: points to the root launcher. Recreate it if you move
  the game folder. Existing shortcuts are not overwritten.
Both Extras are off by default and are applied when you click Play.

EVERY SUBSEQUENT LAUNCH
----------------------
Open Boom Boom Rocket.exe to play immediately. The original package can be moved
or removed afterwards; the launcher never changes or deletes it automatically.

FOLDER LAYOUT
-------------
Boom Boom Rocket/
  Boom Boom Rocket.exe
  resources/             Internal runtime, imported assets, settings and saves
  README.txt
  licenses/

Keep the complete folder together when moving or backing up the game.
resources/assets is the imported base game; resources/userdata holds saves,
DLC and caches. Do not rename numbered userdata folders or redistribute imported
game content. resources/import-manifest.json tracks the import, while the release
manifest and SHA256SUMS.txt describe the immutable supplied files.

DLC / REPAIR / UPDATES
---------------------
For DLC on first launch, use Browse DLC (optional) before or after choosing the
game package, then click Play. Skip DLC clears a queued optional selection.
Alternatively, select Download Content on the main menu to import the optional Rock Pack DLC.
It closes the game before importing and returns to play afterwards. When the
pack is present, the side description says Boom Boom Rock Pack DLC is already installed.
Choose Browse DLC package to import your Boom Boom Rock Pack package.
To repair/re-import game assets, close the game and run
"Boom Boom Rocket.exe" --manage from a terminal. Saves and display settings are
preserved. DLC never blocks first launch.

Extract future updates over the same folder: resources/defaults.toml does not
replace your resources/boom_boom_rocket.toml settings. Native code updates reuse
imported assets. Asset patch updates are applied only when required.

For older Setup/Game releases, extract this build beside the old setup executable
or select the old portable folder in the first-launch window. Compatible assets,
settings, saves and DLC are copied from Game/resources; old files are not deleted.
Conflicting save/config destinations are rejected instead of overwritten.

REQUIREMENTS / LIMITATIONS
-------------------------
Windows 10/11 x64, AVX2-capable CPU, Direct3D 12 GPU and .NET Framework 4.8.
One tested original executable revision is supported; renamed/repacked containers
are accepted if their executable matches. Unsupported revisions are rejected.
Local multiplayer needs two Xbox-compatible controllers and remains experimental.
Xbox Live, system achievement UI, Marketplace and online leaderboards are absent.

CONTROLS
--------
WASD or arrows: D-pad directions. Enter: Start. Esc: pause during play/back in menus.
Space: Bonus Run during play, Start in menus. E or left click: A/confirm.
B, Backspace or right click: B/back. X/Y: X/Y buttons.
Mouse wheel changes songs; left click confirms the highlighted song.
Graphics/audio/timing options and Gamepad/Keyboard Controls are in Help & Options.

SECURITY / LICENCES
-------------------
No admin privileges, system installation, packer or executable self-extraction.
Executables are unsigned and may still trigger antivirus false positives. Do not
disable antivirus protection. Check download origin and supplied SHA-256 checksums.
Do not upload original game packages when reporting issues; redact local log paths.
Our tooling/integration code is MIT; the original/translated game and its assets
are not MIT. Third-party notices and licences are in licenses/. Matching dependency
source accompanies public runtime releases. This is an unofficial project.
