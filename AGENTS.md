# AGENTS.md — working on the Animal Crossing New 3DS port

This repository is a fork of [ACGC-PC-Port](https://github.com/flyngmt/ACGC-PC-Port) (remote `upstream`,
branch `main`), itself built on the [ac-decomp](https://github.com/ACreTeam/ac-decomp) decompilation. The
work here ports the game to the **New Nintendo 3DS** as homebrew (devkitARM, libctru, citro3d), on branch
**`3ds-port`**. The game reads its assets from the player's own disc image at run time; nothing of the game
is in the repository.

## Where things are

| Path | What | Who owns it |
|---|---|---|
| `src/`, `include/` | The decompiled game (C/C++) | upstream: change only under `#ifdef TARGET_3DS` |
| `pc/` | The PC port's platform layer (MIT) | upstream: change only under `#ifdef TARGET_3DS` (one file so far); reused files compile unchanged |
| `platform/3ds/` | **The 3DS port**: `game/` (platform layer), `gfx/` (GX backend on citro3d), `common/` (pure, host-tested modules), `include/` (the shim header), `tools/` | this project |
| `platform/3ds/lib/quicknes/` | QuickNES, the NES games' emulator (LGPL 2.1+): the part the port uses | third-party: changes marked in each file and listed in its README |
| `platform/3ds/res/` | The release's files: the app's icon and banner, the CIA's `ac3ds.rsf`, the player's `README.txt`, third-party notices | this project |
| `README.md` | The front page, for players: installing, playing, troubleshooting, credits | this project |
| `platform/3ds/README.md` | For developers: the builds (player, developer, release), running in Azahar, the developer tools, the changes to shared code | this project |
| `ref/` (gitignored) | Your local files: reference checkouts, an emulator copy, anything taken from your disc image | local only |
| `build/3ds`, `build/3ds-debug` | Player and developer builds | local only |

Your own disc image (GAFE01, revision 0) can sit in the project root: `.gitignore` covers disc images.

## Rules that are not negotiable

1. **Never commit game data**: disc images, anything extracted, converted or dumped from them (textures,
   models, logs with game data), saves. `.gitignore` covers the usual names; check `git status` anyway.
   Never copy a disc image into the repository; to give Azahar's virtual SD card one, link it.
2. **3DS code goes in `platform/3ds/`.** A change to `src/`, `include/` or `pc/` is a last resort: keep it
   minimal, guard it with `#ifdef TARGET_3DS`, and add it to the table in `platform/3ds/README.md` ("Changes
   to the shared code"). Prefer the existing techniques that avoid it: a `TARGET_3DS` entry point next to
   static data the port needs, a per-file `#define` rename in `CMakeLists.txt` (`fread`, `fclose`, `readdir`
   are renamed that way), or a 3DS copy of a PC platform file.
3. **Do the task asked, then report.** Ask before architectural decisions (threading, memory layout, save
   format, build flavours) and before anything hard to undo (history rewrites, deleting the user's files,
   pushing). Never push: publishing is the maintainer's call.
4. **Verify before saying it works**: both builds compile with no new warnings, the host tests pass, and
   the change was seen running in Azahar where it can be. Say what couldn't be checked: Azahar doesn't
   emulate the CPU caches, its timings and performance aren't the hardware's, it can't close a lid, and it
   tolerates GPU programs the real PICA200 hangs on.
5. **Keep the docs true**: `README.md` and the release's `res/README.txt` (anything a player sees) and
   `platform/3ds/README.md` (anything a developer needs).

## Build and test

```sh
# Player build (the default; what is released)
cmake -S platform/3ds -B build/3ds -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake
cmake --build build/3ds                        # build/3ds/ac3ds.3dsx
# Developer build: Testing settings, debug stats, SELECT shortcuts, nes_test
cmake -S platform/3ds -B build/3ds-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake -DAC3DS_DEBUG=ON
cmake --build build/3ds-debug
# Host tests (no devkitARM): every pure module
make -C platform/3ds/common test && make -C platform/3ds/gfx test
make -C platform/3ds/common coverage            # per-file coverage (clang)
# Release package (player build, .3dsx + .cia, readme, licences): build/release/ac3ds-<version>.zip
platform/3ds/tools/make_release.sh --source-url <the repository's address>
```

`AC3DS_DEBUG` is 0 or 1 in every file. Developer-only behaviour is gated on it in code
(`if (AC3DS_DEBUG …)`), and the Testing settings are refused by `settings_offered()` in the player build.

## How the port fits together

- **Boot** (`game/ac3ds_main.c`): New 3DS check (an original 3DS gets a warning and may try on less
  memory), memory (`heap_3ds.c`: all application memory is linear), data folder `sdmc:/3ds/ac3ds/`,
  settings and log, citro3d (`frame_3ds.c`), the disc image (`disc_3ds.c`, then the PC port's `pc_disc.c`),
  the assets (`pc_assets.c`), the sound firmware; then the game's own `boot_main`. Failures go through
  `status3ds_fatal` (plain words, wait for START).
- **Each frame**: the game runs; emu64 turns its display lists into GX calls; `gfx/gx_api*.c` records GX
  state in a `GxbState` with change stamps; `gfx/gx_render.c` turns each draw into citro3d work (the TEV
  translation cache, uniforms, textures, the screen mapping), sending only what changed. Between frames
  (`game/vi_3ds.c`) come input, the hooks (settings, clock, acres, shop, weeds, test flags), the bottom screen and
  the frame's submission.
- **The screens**: the 3DS framebuffers are portrait; `gfx/gx_screen.c` maps GX's 640×480 into the top
  screen (widened, stretched, or 4:3 with bars), folding viewports into the projection. Wide follows the
  game's own widescreen tags (emu64's markers, as on PC): the world seen wider, menus kept 4:3, full-screen
  pictures stretched; top screen only. EFB copies stay on the GPU
  (`gx_efb.c`, `gx_uv.c`).
- **The bottom screen** (`game/bottom_*.c`): its own render target, drawn by the game's own code (the map
  menu, the font, inventory icons, the dialogue's name plate) into a private display list, and redrawn only
  when what it shows changes.
- **Sound**: the game's mixer on core 2 feeding one ndsp channel; NES games on their own channel.
- **NES games**: QuickNES (`lib/quicknes`) behind `game/nes_3ds.cpp`, which keeps the fixNES entry points
  `famicom.cpp` calls; the picture is tiled into a texture each frame (`nes_video_3ds.c`).
- **Saves**: the PC port's GCI code, flushed before close. The Save button goes through the gyroid's own
  scene change (never save the town in place: actors hold objects out of the town's data until it's left).

## Hardware lessons (each cost a long hunt)

- The PICA200 hangs if a vertex shader leaves **any component of a declared output** unwritten (Azahar
  doesn't care). `gx.v.pica` writes every output in full.
- Every texture unit gets a texture, even unused ones (`gx_bind.c` binds the white fallback).
- Fragment-lighting registers keep what the previous program left: `frame_3ds.c` switches them off.
- **Don't hold system service sessions open** (`gsp::Lcd` held open froze the HOME Menu): open, call, close.
- The console writes the framebuffer through the CPU cache: `gfxFlushBuffers()` after drawing a screen.
- The system tick stops while the console sleeps: the game's clock catches up with the RTC
  (`os3ds_clock_frame`).
- `gx_gpu_check.c` reads citro3d 1.7.1's private context: recheck it after a citro3d upgrade.

## Code conventions

- C11 (C++ only where the decomp or a C++ library needs it: `game/nes_3ds.cpp`). One module = one `.c` +
  `.h`; module prefixes (`settings3ds_`, `bottom3ds_`, `gxr_`, `gxb_`, `gxtc_`, …); a module's state in one
  `static struct`.
- The game's global `operator new` is JSystem's (`JKRHeap.cpp`: the current game heap). C++ outside the game
  allocates with `malloc` (`lib/quicknes` uses `blargg_new`).
- **Comment like the PC port** (about 7 % of lines; check with the code itself, not by habit): a one- or
  two-line file header saying what the module is for; a short contract on a public function only when its
  name and types don't say it (units, ownership, who calls it, when); a note where the code would otherwise
  look wrong (a hardware trap, a workaround, where a magic number comes from). No comments that restate the
  code. History, review notes and design rationale belong in commit messages.
- **Pure logic goes in `common/`** (or a pure `gfx/` module) with a host test written first; aim for
  100 % line coverage there (80 % minimum). Hardware glue stays thin and is checked in Azahar and on the
  console.
- Prefer functions that return a new value over mutating arguments (`Settings`, `AcreSelect`,
  `PressHold` are passed and returned by value).
- Files under 800 lines, functions under 50; no magic numbers (a `#define` with a comment saying where the
  value comes from); handle every error, and tell the player in plain words when it's theirs to fix.
- Match the surrounding style: 4 spaces, braces on the same line for control flow and on their own line
  for functions, lines under 120 columns.

## Testing in Azahar

- A portable Azahar copy (in `ref/`, say) keeps its own settings and virtual SD card, away from your own
  setup.
- For automated checks, use a separate copy and SD card rather than the one you play with (it holds your
  save), drive it with posted key events and key-mapped touches ("touch from button" in `qt-config.ini`),
  and capture its window. Restore any setting or file you change for a test.
- Azahar can emulate an original 3DS (`is_new_3ds=false`), which is how the original 3DS's warning and tight
  memory are checked.

## Git

- Conventional commits: `<type>: <description>` (`feat`, `fix`, `refactor`, `docs`, `test`, `chore`,
  `perf`), a body saying what changed and why.
- **Few, larger commits**: a feature, its tests and its docs together.
- Stage only your own changes. Leave untracked local files (such as a `nes_test` flag file) alone.

## Status and next steps

The release plan: playtest on the console (done), a first launch that works for others (done), a clean
release build (done: version, icon, CIA, release script), player documentation (done: the front page and the
release's readme), publishing. The version is `project(ac3ds VERSION …)` in `platform/3ds/CMakeLists.txt`.
Nothing drawn from the game goes into a release; art by others is credited in `res/THIRD-PARTY.txt`.
