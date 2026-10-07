# Reproducible source hooks

`generated.json` contains insertion/replacement operations, not complete
translated game files. Each operation is guarded by the normalized LF input
SHA256 and checked against the complete expected result. SDK/tool revision and
original XEX fingerprint are fixed. `Apply-SourcePatches.ps1` validates the whole
set before writing and is idempotent; unexpected source is rejected.

The four original manual-edit locations are:

- `boom_boom_rocket_init.h`: title-local offline two-player import redirects.
- `boom_boom_rocket_recomp.0.cpp`: judgement clock offset, history reseeding and
  session-start hooks for timing calibration.
- `boom_boom_rocket_recomp.1.cpp`: main/pause descriptor filtering; physical-pad
  hints and title prompt; mouse row/popup bounds; settings click-cycle callback.
- `boom_boom_rocket_recomp.2.cpp`: native PC-setting rows and selection bridge;
  calibration and rocket-label persistence; mouse frame handling; title/gameplay/
  song-screen detection; authored help/Lua reader hooks; offline Download Content
  action routing. Existing functional changes are preserved.

Compared with the known-working source, release regeneration removes only old
ofstream tracing and its read-only diagnostic memory scans, and normalizes line
endings. No AE118 tracing is added. Private before/after copies remain in `out/`.

`sdk.json` patches 20 files of pinned ReXGlue: graphics context rebuild and guest
FPS count (GPU ABI 2), FPS overlay, config access/TOML escaping, window title and
logical-size fitting, modal input capture, host FP exception masking, standalone
title termination, and Windows-safe libmspack source selection.

Not included from the shared development SDK: network sockets/LIVE/netplay/
leaderboards/party services and clock sync, relative mouse/shooter mappings,
other-game D3D12 occlusion patches, optional host FPS limiter, indirect-call debug
tracing, unrelated profile/keystroke/button-filter helpers or memory-table fixes.
BBR's keyboard, profile and local multiplayer code is project-owned under `src/`.

The 15 apparent libmspack changes were materializations of symlink targets, not
code changes. Using `libmspack/libmspack/mspack` avoids those copies entirely.
SDK-derived changes retain the SDK's BSD license; no retail game assets or
complete generated/translated source belong in the public repository.
