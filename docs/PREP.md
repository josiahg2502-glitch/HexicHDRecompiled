# Hexic HD (Xbox Live Arcade) PC port

Static recompilation of Hexic HD (2005, Carbonated Games / Microsoft, title ID 584107D1, XEX version 2.0.1884.0) on the ReXGlue SDK.

## Status (2026-10-29)

- **Playable.** Tested with scripted controller input: main menu, Timed mode (combos score, timer runs out, new-high-score dialog, back to menu), Survival mode (star pieces), the pause menu, Game Options, and Tutorial pages. The attract demo also plays from the menu. See `docs/milestone-playable.png`.
- **Shaders are precompiled:** 8 shaders and 9 pipelines ship in `port/assets/shader_cache`, and all of them are created at startup ("Translated 8 shaders from the storage"). The game is Flash-driven 2D, so that set is small; Marathon play past level 1, starflowers and black pearls haven't been played yet and could add a few.
- **Install:** the player copies their XBLA package (the `584107D1...` file, or the whole `584107D1` folder) into `game\`. The first start unpacks it there (`port/src/stfs_extract.cpp`). Tested from a fresh `dist` folder: 51 files unpacked, game boots.
- `tools\package.ps1` writes `dist\HexicHD` (24 MB, no game data).
- Not verified: audio by ear. The log shows no XMA/audio errors, and the audio worker runs guest callbacks (that is where the FPSCR crash below came from), but nobody has listened to the music (WMA files) yet.

## Layout

| Path | What |
|---|---|
| `N:\ROMS\Xbox 360\Hexic HD\` | unpacked game files (default.xex, *.swf, *.wma, *.xml). Source: `Downloads\Hexic HD.rar` (an STFS PIRS package). |
| `N:\Projects\Monopoly\rexglue-sdk` | the SDK, shared with the Monopoly port (same pinned commit c94f5eb). This port's SDK changes are in `patches/rexglue-xbla-fixes.patch`; Monopoly's are in its own `patches/`. |
| `port/` | the game: `hexic_manifest.toml`, `config/hexic.toml` (codegen function list), `src/` (app, crash logger, script input, STFS installer), `assets/shader_cache`, `generated/` (codegen output, not in git) |
| `tools/` | `env.cmd`, `sdk_install.cmd` (rebuild SDK runtime/codegen and copy into its install tree), `stfs.py` (unpack a package), `ppcdis.py` (disassemble the dumped image with capstone), `find_missing_funcs.py` + `fix_missing_funcs.ps1`, `run.ps1`, `live.ps1` + `nav.ps1`, `merge_shader_cache.py`, `warm_shader_cache.ps1`, `package.ps1` |

## Build and run

```
tools\sdk_install.cmd                                         # only after SDK changes
cd port
..\tools\env.cmd rexglue codegen hexic_manifest.toml          # only after config or SDK changes
..\tools\env.cmd cmake --preset win-amd64-release             # once
..\tools\env.cmd cmake --build --preset win-amd64-release
cd ..
tools\package.ps1
```

Run from source: `port\out\build\win-amd64-release\hexic.exe --game_data_root="N:\ROMS\Xbox 360\Hexic HD"`.

Unattended testing: `tools\live.ps1 start`, then `tools\nav.ps1 "A:4 DOWN:1 A:2" -Shot name`, then `tools\live.ps1 stop` (closes gracefully, which flushes shader storage). Merge new shaders with `python -I tools\merge_shader_cache.py port\assets\shader_cache userdata\cache` and rebuild.

Dump the decrypted image for analysis: set `REXGLUE_DUMP_IMAGE=N:\Projects\HexicHD\image.bin` and run codegen.

## Fixes and findings

- **XBLA images load at 0x92000000**, in the 4 KB-page heap at 0x90000000. The SDK always put its recompiled-code function table (right after the image) in the 0x80000000 heap, which failed ("Failed to allocate function table at 922A5000"). `Memory::InitializeFunctionTable`/its release now use the heap that contains the address.
- **Host FP exceptions on the audio thread:** the game calls `KeEnableFpuExceptions`, then its audio callback crashed with STATUS_FLOAT_INEXACT_RESULT (0xC000008F) inside guest float code. `XHostThread` contexts never ran `fpscr.InitHost()`, so their saved csr was 0 and the first FPSCR write loaded an MXCSR with every exception unmasked. `XHostThread::Execute` now initializes it like guest threads do. This also applies to Monopoly the next time it is rebuilt against the SDK.
- **Codegen config** (`port/config/hexic.toml`): one `this`-adjust thunk target with no .pdata (0x92194DC0), and 28 vtable/callback targets that codegen folded into neighbours, found by `tools/fix_missing_funcs.ps1` (converged in 7 passes).
- `port/src/crash_log.cpp` logs unexpected host exceptions with a native stack to `crash.txt` next to the exe; symbolize with `llvm-symbolizer --obj=hexic.exe 0x140000000+offset`.
- The SDK's `install` target fails on `unit_tests` (missing xxhash include path, pre-existing); `tools\sdk_install.cmd` builds and copies the three needed targets instead.
- On the menu, the new-high-score dialog asks for the Guide button to sign in; "Done" dismisses it.

## Next steps

1. Listen to audio (music and effects) and fix anything wrong with WMA/XMA playback.
2. Play Marathon further (starflowers, black pearls, level ups, bombs) and merge any new shaders.
3. Icon, and remember window size/position; maybe a launcher like Monopoly's if wanted.
4. Leaderboards/Achievements screens need a signed-in profile; check what they show.
