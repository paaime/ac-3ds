# platform/3ds — the New 3DS port

All 3DS-specific code for the Animal Crossing (GAFE01) port lives here. Shared decomp and PC-port code is
only touched under `#ifdef TARGET_3DS`; [Changes to the shared code](#changes-to-the-shared-code) lists every
such change. Installing and playing are in the [repository's README](../../README.md).

```
platform/3ds/
  CMakeLists.txt  the whole game: the decomp, the reused pc/src files and this directory
  cmake/          game_sources.cmake: the PC port's source list, refreshed by tools/sync_game_sources.py
  include/        pc_platform.h, the 3DS replacement for pc/include/pc_platform.h (force-included,
                  same include guard); SDL-free PC feature headers; sdl_compat/ for jaudio's os.c
  game/           the platform layer: boot and start-up messages (ac3ds_main.c, status_3ds.c,
                  disc_3ds.c), memory (heap_3ds.c), OS and clock (os_3ds.c), frames (vi_3ds.c,
                  frame_3ds.c), input (pad_3ds.c), sound (audio_*.c, ndsp_stream_3ds.c), saves
                  (save_3ds.c), NES games (nes_3ds.cpp, nes_*_3ds.c), settings (settings_3ds.c), the bottom screen
                  (bottom_*.c, town_map_3ds.c, quick_save_3ds.c), developer tools (trace, profiler,
                  texture dump, nes_test)
  gfx/            the GX backend emu64 draws through, on citro3d: gx_api*.c (the GX SDK), gx_render.c
                  (draws), gx_bind.c (textures), gx_tev.c (TEV -> PICA combiner), gx_uniforms.c,
                  gx_texcache.c, gx_efb.c (EFB copies), gx_screen.c / gx_uv.c (the screen mapping),
                  gx.v.pica (the vertex shader); developer checks gx_gpu_check.c, gx_debug.c
  common/         pure modules with host tests (settings file, layouts, timers, texture decoding and
                  tiling, audio bookkeeping, disc checks, text wrapping, ...), plus console_row
  lib/quicknes/   QuickNES, the NES games' emulator: third-party code (LGPL 2.1 or later); its README says
                  what was taken, what changed and what a release must include
  res/            the release's files: the app's icon, the CIA's settings (ac3ds.rsf),
                  the player's README.txt and the third-party notices
  tools/          sync_game_sources.py, gdb_profile.py (statistical profiling through Azahar's GDB
                  stub), make_release.sh (the release package)
```

On an original 3DS or 2DS the game starts on what the `.cia` gets: 75 MB (its 96 MB mode less the program)
against the usual 80, so 67 MB of heap (the town's peak was 63.7 MB) and an 8 MB pool.

## Prerequisites

- devkitPro with the `3ds-dev` group (devkitARM, libctru, citro3d, picasso, 3dslink): install the devkitPro
  pacman `.pkg` from <https://github.com/devkitPro/pacman/releases>, then run `sudo dkp-pacman -S 3ds-dev`.
- CMake and Ninja (`brew install cmake ninja`).
- For the host tests: any C compiler (clang for the coverage reports).

## Build

```sh
cmake -S platform/3ds -B build/3ds -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake
cmake --build build/3ds                      # → build/3ds/ac3ds.3dsx (~90 s clean on an 8-core Mac)
```

That is the **player build**. The **developer build** builds into a folder of its own and is named
"Animal Crossing 3DS (developer)" in the Homebrew Launcher:

```sh
cmake -S platform/3ds -B build/3ds-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake -DAC3DS_DEBUG=ON
cmake --build build/3ds-debug                # → build/3ds-debug/ac3ds.3dsx
```

It adds:
- the Options page's **Testing** settings: Fast GPU flush, Sound on one core (the mixer normally runs on
  core 2), Detailed log (the PC port's diagnostics and, from the next start, the first two drawing frames
  checked draw by draw and a watchdog that logs where the game is stuck), Dump textures;
- the **stats** (hold Options 2 s, or SELECT + R): frame rate, citro3d CPU/GPU time, draws, the command
  buffer, textures, emu64's command counts, memory, sound;
- SELECT + L, which writes one frame's draws to the log;
- `nes_test`: a text file in `sdmc:/3ds/ac3ds/` holding a number from 1 to 19 makes SELECT + START in town
  start that NES game.

Both builds use the same data folder (`settings.ini`, saves, `ac3ds.log`); the player build counts the
Testing settings as off.

Host tests (Mac/Linux, no devkitARM needed):

```sh
make -C platform/3ds/common test             # the pure modules
make -C platform/3ds/gfx test                # the GX backend's pure parts (TEV, alpha test, screen mapping, ...)
make -C platform/3ds/common coverage         # either, with a clang coverage report
```

### Release build

```sh
platform/3ds/tools/make_release.sh --source-url https://…   # → build/release/ac3ds-<version>.zip
```

It configures and builds the player build in its own folder (`build/3ds-release`), makes the `.cia`, and packs
both with the player's `README.txt` and the licences (`res/`). The zip holds exactly those files, so nothing
else can slip in. With `--source-url`, it also draws `ac3ds-<version>-fbi-qr.png` for FBI's Remote Install:
the address of `ac3ds.cia` in the GitHub release tagged `v<version>`, so upload the zip, `ac3ds.cia` (from the
`ac3ds-<version>` folder) and the QR code there. It needs `qrencode` (`brew install qrencode`, or `QRENCODE`);
without it the script says so and goes on. The sources must be committed, since the readme names the commit; `--allow-dirty` makes a
test package named `-dirty`. The version is `project(ac3ds VERSION …)` in `CMakeLists.txt`. It shows on the
loading screen, in the log's first line and in the Homebrew Launcher.

- **The `.cia`** needs two tools devkitPro doesn't ship: `makerom` (3DSGuy/Project_CTR) and, for its banner,
  `bannertool` (carstene1ns/3ds-bannertool). The script finds them through `MAKEROM`/`BANNERTOOL`, on `PATH`, or in
  `ref/bin/ctrtools/`. Build them from source there:
  `make deps && make` in `Project_CTR/makerom`, and `cmake -B build && cmake --build build` for bannertool.
  Each needs a one-line fix:
  - bannertool, for Apple clang: in `source/3ds/lz11.cpp`, `u8 pad[4] = {0};`.
  - makerom 0.19.0: in `src/user_settings.c`, `version.h`'s `#define VER_MINOR 19` shadows the version array's
    index, so the minor version went to `titleVersion[19]` (past the array's end) and the CIA said 0.0.0. Write
    `titleVersion[1]` in those three places.
- **`res/ac3ds.rsf`** sets the CIA's title:
  - title id `000400000fac3d00` and the homebrew boot logo;
  - New 3DS mode: 124 MB, 804 MHz, L2 cache, core 2; on an original 3DS, its largest mode, 96 MB;
  - the services the port uses.

## Running

- **On the console:** copy `build/3ds/ac3ds.3dsx` to `sdmc:/3ds/` and start it from the Homebrew Launcher
  (opened through Download Play or another title, for the full memory), or install a release's `ac3ds.cia`
  with FBI. Over Wi-Fi, an FTP server on the console (ftpd) saves taking the SD card out:
  `curl -T build/3ds/ac3ds.3dsx "ftp://$IP:5000/3ds/"`.
- **In Azahar:** link the disc image into the emulator's virtual SD card rather than copying it
  (`ln -s "$PWD/Animal Crossing (USA).iso" ".../sdmc/3ds/ac3ds/"`), then
  `open -n -a Azahar.app --args "$PWD/build/3ds/ac3ds.3dsx"`, or `azahar -i ac3ds.cia` and start it from the
  game list. Launch it through `open`: starting the binary inside the app bundle shows a modal warning.
  Azahar's timings aren't the hardware's, it doesn't emulate the CPU caches, `osSetSpeedupEnable` does nothing
  there, and it runs GPU programs the real PICA200 hangs on.

## Developer tools

**Testing in Azahar with a keyboard:** a portable Azahar copy (`ref/bin/azahar/portable/`) keeps its own
settings and virtual SD card. Azahar's keyboard defaults: Circle Pad = arrow keys, A/B/X/Y = A/S/Z/X, D-pad =
T/F/G/H, L/R = Q/W, ZL/ZR = 1/2, START = M, SELECT = N; a mouse click on the bottom screen is a touch.

**Dumping the game's textures** (to pick art for the bottom screen, say): in the developer build, Options →
Testing → Dump textures saves each texture the game draws, once, as a TGA file in `sdmc:/3ds/ac3ds/dump/`.
It's your disc's own art: keep it out of the repository.

**Profiling:** `tools/gdb_profile.py` samples the game through Azahar's GDB stub (set both `use_gdbstub=true`
and `use_gdbstub\default=false` in its `qt-config.ini`) and groups the samples by what the frame was doing.

## Changes to the shared code

Eight files outside `platform/3ds/` change, each only under `#ifdef TARGET_3DS`, so the PC build is
unchanged. A new change is a last resort, and gets a row here. The names they share with the port
(`g_ac3ds_fast_text`, `ac3ds_save_return`, `g_ac3ds_boat_ride`) are declared in the shim header,
`include/pc_platform.h`.

| File | Change | Why |
|---|---|---|
| `include/libultra/libultra.h` | Includes `<strings.h>` and skips its own `bcmp`/`bcopy`/`bzero` declarations | newlib declares them with `const` and `size_t`, and both in one file don't compile. The definitions in `pc_misc.c` and `pc_stubs.c` already match newlib's |
| `pc/src/pc_save_bswap.c` | `swap_mQst_base` repacks only the quest header's 16 bitfield bits (`common/quest_bits.c`) | ARM GCC puts `time_limit` at byte 2, as the GameCube does. The PC code, written for MinGW's layout (byte 4), zeroed every quest deadline's seconds and minutes at each load and save |
| `src/game/m_map_ovl.c` | `mMP_3ds_refresh`, `mMP_3ds_draw_grid`, `mMP_3ds_draw_panel`: the map menu's data setup and drawing, with a private state and menu info, through a projection the caller gives. `mMP_3ds_available` (whether the map can show) and `mMP_3ds_select` (the cursor on a tapped acre) | The bottom screen's map is the game's own map code, whose functions are static in that file |
| `src/game/m_msg_cursol.c_inc` | With "Faster text" on, the message cursor sets `mMsg_STATUS_FLAG_FAST_TEXT` from a message's first letter, as pressing B does | The game's own fast speed, so pauses, choices and voices behave as with B |
| `src/game/m_msg.c` | `mMsg_3ds_draw_nameplate`: the dialogue's name plate, tinted, under the caller's modelview | The Options title on the bottom screen sits in the game's own name plate, whose model and texture are static in `m_msg_data.c_inc` |
| `src/actor/npc/ac_npc_restart_schedule.c_inc` | The save room's "continue" asks the port whether the player came from the Save button. If so, they go back to the spot they left, as from an NES game, not out of their house's door. Quitting to the title forgets the spot | The Save button saves anywhere in town through the gyroid's own scene change and save room |
| `src/actor/ac_boat_demo_move.c_inc` | The boat ride sets `g_ac3ds_boat_ride` while it turns borderless acres off for its scripted acre changes | "Continuous camera" is applied every frame, and leaves the switch to the ride meanwhile |
| `src/actor/ac_animal_logo.c` | The PC port's title menu (Start Game, Options, Quit Game) is built only without `TARGET_3DS` (`aAL_PC_TITLE_MENU`): the 3DS keeps the GameCube's blinking "Press START!" | HOME closes the game on 3DS, and the port's options are on the bottom screen |

Everything else compiles unchanged, by other means:
- `jaudio_NES/internal/os.c`'s `#include <SDL.h>` finds a 3DS compatibility header on that file's include path
  alone (`include/sdl_compat/`).
- Name clashes are settled in the shim header: libctru's global `osGetTime` is renamed away from the game's,
  and `<errno.h>` comes first, so `osContPad.h`'s `errno` field works as on PC.
- Per-file renames in `CMakeLists.txt` replace a reused PC function without touching its file:
  `pc_stubs.c`'s `OSGetSoundMode`/`OSSetSoundMode` (→ `game/audio_3ds.c`), `pc_mtx.c`'s `C_MTXOrtho`
  (→ `game/mtx_3ds.c`), `fclose` in `pc_m_card.c` and `pc_card.c` (→ `game/save_3ds.c`, which flushes first),
  `fread` and `readdir` in `pc_disc.c` (counted reads; hidden files such as macOS's `._` skipped), and
  `pc_misc.c`'s `sins`/`coss` (→ libultra's own table versions, which the 3DS build compiles).
