# Portable launcher

The root C#/.NET Framework 4.8 executable contains only launcher/import logic,
PC text labels and player-name suggestions. Runtime EXE/DLL files are ordinary
files in `resources/`, not a packed or self-extracting payload. No new framework,
service, registry installation, admin privilege or internet connection is used.

`setup/launcher/PortableGame.cs` reuses the existing STFS reader and supported
XEX fingerprint. Base package validation reads the executable inside the package
before extraction. Imported assets are validated again before commit. Discovery
is top-level beside the launcher; an explicitly selected folder permits up to
four nested levels, with file/folder limits. Multiple matches require a choice.

`resources/import-manifest.json` tracks the supported revision, text-patch
version and installed DLC IDs. A normal launch checks the marker and presence
of required files, not the original package or a full game hash. Repair always
revalidates/imports the selected package. Increment `AssetPatchVersion` when
imported text patches change; native-only updates do not require reimport.

Runtime-compiled modifications (recompiled hooks, authored Lua replacements,
input, display, timing, multiplayer and runtime menu-label changes) are separate
from appended PC text labels applied during asset import. The native runtime
continues updating its existing menu text and labels before loading game data.

Imports use staged files and an exclusive per-installation lock. A durable
rename journal enables rollback after a crash/process kill during commit;
abandoned extraction stages are removed on next preparation. Cancellation is
honoured before commit. Commit itself completes or rolls back, preserving saves
and configuration. Recovery failures leave the journal/backups for inspection.
Original packages are opened read-only and never removed.

Older `Game/resources` (or early `Game`) installations are copied, not moved.
Save/config collisions are rejected. Migration leaves the old directories and
setup executable intact; remove them manually only after checking your new copy.
Imported manifests contain no original package path or machine-specific path.

Download Content on the main menu opens the same launcher's Rock Pack importer
with `--dlc --wait-pid=...`, closing the game normally first. When the pack is
present its side description reads Boom Boom Rock Pack DLC is already installed. First launch also
offers optional DLC selection before or after the base package; it validates and
queues DLC if needed, imports packages, then waits for an explicit Play click.
Subsequent normal launches still bypass the interface and start immediately.
Browse Package is disabled after a successful first import; repair remains
available in management mode. Opt-in first-setup Extras apply on Play: fullscreen
updates only that portable TOML setting through the existing journal, and a
desktop shortcut uses Windows' built-in shortcut API to target the root launcher.
Neither option runs unless selected; an existing shortcut is never overwritten.
Skip DLC clears the queue. No folder-picker button is displayed; folder drops
remain supported. Repair remains available by closing the
game and running the launcher with `--manage`; there is no extra Help & Options
entry. The importer refuses writes while its native runtime is active. Local
multiplayer and save layout are unchanged. The import windows reuse the original
project-owned neon wordmark, firework and skyline drawing from the old setup.

## Antivirus review

The earlier unsigned setup embedded a compressed archive containing executable
runtime files, extracted them and started another executable. That pattern may
attract heuristic detections, but no detection sample/vendor report was supplied
and it does not establish the cause or establish malware. The new package removes
the executable archive/self-extraction path. Files are not packed/obfuscated,
the manifest requests `asInvoker`, and SHA-256 checksums accompany the ZIP and
immutable release files. The launcher and runtime remain unsigned. No antivirus
guarantee or independent security scan is claimed; never disable protection.

## Checks

`tools/Test-Launcher.ps1 -AssetsDirectory YOUR_PRIVATE_EXTRACTED_ASSETS` creates
private test packages under ignored `out/`. It checks discovery/multiple matches,
import, repeat launch without source, DLC extraction, repair, settings/save
preservation, invalid/unsupported revision rejection, cancellation, rollback,
hard-process-interruption recovery and migration. Its WinForms event tests exercise
file selection, folder selection, file/folder drops, automatic lookup and a
multiple-match choice, with a private launch probe. It never ships fixtures.
Synthetic DLC fixtures test extraction/header handling, not playable DLC content.
Native input/profile tests remain separate. Real user UI/antivirus feedback is
still useful; the historical installer tests target the old layout only.
