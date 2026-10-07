Boom Boom Rocket XBLA PC Recomp
==============================
Testing release v0.9.1 - PORTABLE RELEASE LAYOUT
Experimental: two real controllers still need user testing.

INSTALL
-------
Extract the whole ZIP into the writable folder where you want to keep the game.
Run Setup Boom Boom Rocket.exe. Select your original Xbox 360 game package
and choose the Boom Boom Rock Pack DLC package if you have it. Click Install game.
Setup creates Game when needed. Choose Play now, or open Game and double-click
Boom Boom Rocket.exe. Setup, README.txt and licenses stay in the release root.

The setup includes the PC runtime only. Your own game and DLC are imported
locally. Package files can be extensionless; do not select a ZIP or default.xex.
Windows 10/11 x64 with .NET Framework 4.8 is required for setup and launcher. No download,
administrator access, registry installation, or separate BAT file is needed.

SUPPORTED GAME REVISION
-----------------------
The PC runtime is already recompiled for one tested Xbox executable layout.
Setup does not generate a new runtime from your selected package. It identifies
the extracted default.xex using SHA256 (not the package filename/container):
B1BC45589CEC79E375E23FC48C122FA5902A2DFD6E3C9C765064DCD9E1A8709B
Unsupported executables are rejected before replacing installed files. Use your
legally obtained original base-game package. Other revisions need separate
compatibility testing; not every metadata-only modification is necessarily bad.

ADD DLC LATER
-------------
Run Setup Boom Boom Rocket.exe again from the same portable folder.
It detects the installed game, so you only need to select the optional DLC.
Your saves, achievements, existing DLC and display settings are preserved.
On a fresh installation, setup picks two different player names from the
included name list. Names stay with that portable copy; updates and DLC do not
reroll them. Names do not change the User/Player2 save-folder identities.

PORTABLE DATA
-------------
Before setup:
Boom Boom Rocket XBLA PC Recomp\
  Setup Boom Boom Rocket.exe    Installer; also adds DLC later
  README.txt                   This guide
  licenses\                    Third-party notices and license texts

After setup:
Boom Boom Rocket XBLA PC Recomp\
  Setup Boom Boom Rocket.exe
  README.txt
  licenses\
  Game\
    Boom Boom Rocket.exe        Play launcher with neon rocket icon
    release-manifest.json       Release metadata and runtime file hashes
    resources\
      boom_boom_rocket.exe      Internal native game runtime
      *.dll                    Matching runtime/GPU libraries
      boom_boom_rocket.toml     Display and runtime settings
      tools\                   Required runtime import helper
      assets\                  Your locally imported base game
      userdata\                Saves, achievements, DLC and shader cache
        5841086A\profile\User\    Player 1 save data
        5841086A\profile\Player2\ Player 2 save data (when needed)
      logs\                    Runtime diagnostic logs (when generated)

Move or back up the whole portable folder, including setup and Game.
There are no hard-coded installation paths in the launcher. Keep the runtime
DLLs beside Game\resources\boom_boom_rocket.exe. Keep the play launcher inside
Game beside resources. The numbered folders in userdata are required by the
game's Xbox content layout; its generated readme explains each location.
The public ZIP does not contain the original package, default.xex or game/DLC
assets. Setup imports these from your own packages into this private copy.
Do not redistribute a populated Game folder; share the original tool ZIP.

Use a fresh extracted folder when testing this layout. Older releases used a
different folder structure and are not automatically migrated or removed.

CONTROLS
--------
Help & Options now has separate Gamepad Controls and Keyboard Controls pages.
Xbox-compatible controllers remain supported. Keyboard and mouse control player 1:
W/S/A/D or arrow keys   Up / Down / Left / Right
Enter or Space          Start (including pause)
Esc while playing      Start (pause)
Esc in menus           B (back; resumes from the pause menu)
E or left mouse button  A (confirm)
B, Backspace or
right mouse button     B (back)
X / Y                  X / Y

Native menu rows support pointer selection and left-click activation.
On song lists, use the mouse wheel to move up/down; left-click confirms the
highlighted song. Moving the pointer does not change songs. Keyboard and
controller navigation still work. Fast wheel bursts are paced and limited.
The title screen says Press Any Key without a controller, or Press START with one.
Controller-button icons and their footer words appear only with a controller.
The Gamepad Controls diagram remains. Rocket labels automatically use ABXY
with a controller or arrows without one, unless you save a manual label choice.
Repeated left clicks on a Settings value advance through every choice and wrap.
E/A saves Settings; right-click or Esc goes back. During play, mouse buttons
still act as A/B. Keyboard and controller navigation remain available.
Controls release when the game loses focus or an overlay captures input.
ReXGlue F3/F4/F7/backtick overlays are disabled. VSync is on by default.
For troubleshooting, close the game and set bbr_runtime_shortcuts = true in
Game\resources\boom_boom_rocket.toml, then relaunch. Set it back to false to disable them.
This switch does not remove native game settings, the version label or Show FPS.

TIMING AND DISPLAY
------------------
Help & Options > Timing Calibration adjusts note judgement from -200 to +200 ms
in 10 ms steps. Positive accepts later hits; negative accepts earlier hits.
Try a familiar song, adjust, and retry. This is manual calibration, not an
automatic latency measurement. A/E saves, B/Esc cancels, X resets to zero.
The saved offset applies when the next song starts (including a restart).
Music playback and visual animation speed do not change.
Settings > Reset Display Settings restores 1280x720 windowed, render scale 1,
VSync on and Show FPS off. It does not reset saves, audio levels or player names.

LOCAL MULTIPLAYER (TEST BUILD)
-----------------------------
Connect two Xbox-compatible controllers before starting the game. Select
Multiplayer, then Battle or Endurance. A local Player 2 profile is assigned
automatically to controller 2; there is no Xbox Live login or account required.
Each profile has its own save file inside Game. Player 1's keyboard/mouse
bindings remain unchanged and supplement controller 1, not controller 2.
Keyboard plus only one physical controller is not a separate-player layout yet.
Keep both controllers connected during a match. Reconnect and restart the game
if joining after launch or a disconnection does not recover correctly.
The automated test used two independent simulated pads; physical controller
assignment, hot-plug recovery and long matches still need testing.
Achievements retain the existing shared runtime store; this is not a second
Xbox account or a separate player-two achievement service.

COMPATIBILITY
-------------
Single-player, controller/keyboard/mouse input, audio, DLC, pause/resume, PC display options,
portable saves and achievement unlocks are supported. Xbox Live leaderboards,
the system Achievements screen and Marketplace are unavailable. Local
Battle and Endurance are enabled experimentally through the original menus.
The new-profile welcome acknowledgement is skipped; profiles are still created
and saved normally. Existing main-menu and pause-menu changes are preserved.

Setup validates and extracts both packages before replacing installed files.
Cancel stops extraction safely; the final short file commit cannot be cancelled.
Close the game before using setup on that installation.

Third-party notices and license texts are in licenses\.
Original tool/launcher/integration code is MIT licensed; this does not license
the original or translated Boom Boom Rocket game code/assets. FFmpeg and
libmspack are LGPL libraries in the runtime. Matching library sources and
rebuild/relink instructions are in the separate dependency-source archive
provided alongside this tool. No original game files are needed to rebuild
those libraries. See licenses\THIRD_PARTY_NOTICES.txt.
