# Animal Crossing Nintendo 3DS Port

A native port of **Animal Crossing** (GameCube, USA) to the New Nintendo 3DS family, as homebrew. The game's
original C code, from the [ac-decomp](https://github.com/ACreTeam/ac-decomp) decompilation, runs on the 3DS's
ARM11, and the GameCube's graphics API is translated to the 3DS's PICA200 GPU through citro3d. It is a fork of
[ACGC-PC-Port](https://github.com/flyngmt/ACGC-PC-Port), the PC port it builds on.

**No game data is included.** The game reads everything from your own disc image of Animal Crossing (USA)
when it starts. This repository holds only source code.

Supported version: **GAFE01, revision 0 (USA)**.

> **Still in development.** This is an early version (0.1): expect bugs. Feedback, bug reports and ideas are all
> welcome: see [Feedback](#feedback).

## Features

- **About 60 fps** in most of the town on a New 3DS, and around 40 in its busiest views. The game keeps its
  normal speed even when the frame rate drops.
- **The town map on the bottom screen**, the game's own. Tap an acre to see what's there.
- **Options**: skip Mr. Resetti, a continuous camera, sharp textures, a true widescreen view, faster text,
  and more.
- **Save from anywhere in town** with the gyroid button on the bottom screen. You come back where you were.
- **NES games** (the NES furniture) at full speed with their sound, through QuickNES.
- **Saves compatible** with Dolphin and the PC port (GameCube GCI files): bring your town along.

## Requirements

- A **New 3DS, New 3DS XL or New 2DS XL** with custom firmware ([Luma3DS](https://github.com/LumaTeam/Luma3DS))
  and the Homebrew Launcher, or FBI to install the `.cia`.
  An **original 3DS or 2DS** hasn't been tested: the game warns you and lets you try (see below).
- **Your own disc image** of Animal Crossing (USA), as `.iso`, `.gcm` or `.ciso`. RVZ and NKit images must be
  converted to ISO first (in Dolphin: right-click the game, Convert File).
- For sound, the **DSP firmware dump**, `sdmc:/3ds/dspfirm.cdc`. If you don't have it: open the Rosalina menu
  (L + Down + SELECT), Miscellaneous options, Dump DSP firmware.

## Installation

1. Download the latest release from the [Releases](../../releases) page.
2. On the SD card, make the folder `sdmc:/3ds/ac3ds/` and put your disc image in it.
3. Then either:
   - **`ac3ds.cia`**: install it with FBI. *Animal Crossing 3DS* appears on the HOME Menu.
   - **`ac3ds.3dsx`**: copy it to `sdmc:/3ds/` and start it from the Homebrew Launcher. Open the Homebrew
     Launcher through a game or Download Play, not from the Rosalina menu, so the game gets enough memory.

The first start reads the disc image for a few seconds. If anything is wrong, the bottom screen says what and
how to fix it.

### On an original 3DS or 2DS

It hasn't been tested. Use the `.cia`,
which gets the most memory an original 3DS can give (75 MB, just under the 80 MB the game usually wants).
Expect it to be slow, and to stop in busy places if it runs out of memory: save often.

## Playing

### Controls

| 3DS | GameCube |
|---|---|
| Circle Pad | Control stick |
| C-Stick (New 3DS) | C-stick |
| A, B, X, Y | A, B, X, Y |
| L, R | L, R |
| ZL or ZR | Z |
| D-pad | D-pad |
| START | START |

In an NES game, hold **L and R and press ZL** to leave (L + R + Z on the GameCube). SELECT isn't used.

### The bottom screen

Once you're in your town it shows the **town map**. Tap an acre to see its houses and owners; the map comes
back to your acre after 15 seconds. Two round buttons sit in the top-right corner:

- **Save** (the gyroid): saves as the gyroid in front of your house does, with the same "Save & Continue /
  Save & Quit" choice, then puts you back on the spot you left. It appears only where the gyroid could save.
- **Options**: the port's settings; tap **Back** for the map.

| Setting | What it does |
|---|---|
| Skip Mr. Resetti | No lecture after a session that ended without saving. |
| Picture shape | Wide (the default) shows more of the world at the sides, with menus at their own proportions; 4:3 keeps the GameCube's picture, with bars at the sides; Stretch stretches the 4:3 picture over the screen. |
| Continuous camera | The camera follows you from one acre to the next (on by default). Off, it stops at the acre's edge and slides over, as on the GameCube. |
| Sharp textures | No smoothing on the 3D world's textures. |
| Faster text | Dialogue at the game's fast speed from the first letter, as if B were pressed. |
| Nookington's without a visitor | The shop's last upgrade normally needs a shopper from another town. |
| No weeds | Every weed is pulled each time you enter your town, and none grow |
| Bottom screen light saver | The bottom screen goes dark after 10 seconds without a touch. |

Settings are saved in `sdmc:/3ds/ac3ds/settings.ini`.

### Saves

Your town is saved in `sdmc:/3ds/ac3ds/save/card_a/DobutsunomoriP_MURA.gci`, with three backups
(`.bak1` to `.bak3`). It's a GameCube save file, the same format as Dolphin's GCI exports and the PC port's:
copy a town in or out to play it elsewhere.

## Not available

- The **Game Boy Advance link**: the island's GBA features, the e-Reader, and visiting another town's island.
- Other regions and later revisions of the game: only GAFE01 revision 0.

## Troubleshooting

- **"Not enough memory"**: start the Homebrew Launcher through a game or Download Play rather than the Rosalina
  menu, or install the `.cia`.
- **No sound**: the DSP firmware dump is missing (see Requirements). The game tells you, then plays silently.
- **"…isn't Animal Crossing"**, or another region's or a later release's version: only the USA version's first
  release (GAFE01, revision 0) works. Dolphin can check a dump: right-click the game, Properties, Verify.
- **The island is black and you fall through the ground**: this happens if the town was renamed with a save
  editor that didn't rename the island's copy of the town name. Set the island's town name to your town's name
  in the editor.
- Anything else: the log is `sdmc:/3ds/ac3ds/ac3ds.log`. Attach it to a bug report.

## Feedback

The port is still in development, and every report helps: bugs, crashes, slowdowns, anything that looks wrong,
and ideas too. Open an [issue](../../issues) with:

- your console (New 3DS, New 3DS XL, New 2DS XL, or an original 3DS or 2DS) and whether you use the `.cia` or
  the `.3dsx`;
- what you did and what happened (a photo or a short video helps);
- the log, `sdmc:/3ds/ac3ds/ac3ds.log` (if possible)

Pull requests are welcome too.

## Building from source

Only needed to change the code. You need devkitPro with the `3ds-dev` group, CMake and Ninja.

```sh
# Player build → build/3ds/ac3ds.3dsx
cmake -S platform/3ds -B build/3ds -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake
cmake --build build/3ds

# Developer build (debug stats, testing settings, shortcuts) → build/3ds-debug/ac3ds.3dsx
cmake -S platform/3ds -B build/3ds-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake -DAC3DS_DEBUG=ON
cmake --build build/3ds-debug

# Host tests of the pure modules (no devkitARM needed)
make -C platform/3ds/common test && make -C platform/3ds/gfx test

# Release package (.3dsx, .cia, readme, licences) → build/release/ac3ds-<version>.zip
platform/3ds/tools/make_release.sh --source-url <this repository's address>
```

The [3DS port's README](platform/3ds/README.md) covers the details: what the developer build adds, testing in
the Azahar emulator, the tools the `.cia` needs, and the developer tools.

## How it works

- **The game** is the decompilation's C code, compiled for the ARM11 with devkitARM. The few changes it needs
  for the 3DS are marked `#ifdef TARGET_3DS`.
- **Graphics**: the game draws through emu64, its N64-style display-list interpreter, which calls the GameCube's
  GX API. A GX backend on citro3d records that state and turns each draw into PICA200 work. The TEV colour
  combiners become PICA combiners; fog, EFB copies and the screen mapping stay on the GPU.
- **The bottom screen** is drawn by the game's own map-menu code into a second render target.
- **Sound**: the game's audio mixer runs on the New 3DS's extra core and feeds the DSP through ndsp.
- **Disc and saves**: the PC port's disc-image reader and GCI save code, unchanged.
- **NES games** run on [QuickNES](https://github.com/libretro/QuickNES_Core), the part of it the port needs.

## Credits

- **[ACreTeam](https://github.com/ACreTeam)** for [ac-decomp](https://github.com/ACreTeam/ac-decomp), the
  complete C decompilation of Animal Crossing that everything here stands on.
- **[flyngmt](https://github.com/flyngmt)** for [ACGC-PC-Port](https://github.com/flyngmt/ACGC-PC-Port), the PC
  port whose platform layer, disc reader and save code this port reuses.
- **Shay Green (blargg)** and the **libretro** contributors for QuickNES.
- **[devkitPro](https://devkitpro.org)** for devkitARM, libctru and citro3d.
- The **[Azahar](https://azahar-emu.org)** team, whose emulator made most of the testing possible.

## License

- The decompilation is CC0 1.0 (public domain); the PC port's layer is MIT: [LICENSE](LICENSE).
- The 3DS port (`platform/3ds`) is MIT: [platform/3ds/LICENSE](platform/3ds/LICENSE).
- QuickNES (`platform/3ds/lib/quicknes`) keeps its GNU LGPL 2.1 or later:
  [its licence](platform/3ds/lib/quicknes/LICENSE) and [its changes](platform/3ds/lib/quicknes/README.md).

Animal Crossing is a trademark of Nintendo. This project is not affiliated with or endorsed by Nintendo, and it
contains no game data: you need your own copy of the game.