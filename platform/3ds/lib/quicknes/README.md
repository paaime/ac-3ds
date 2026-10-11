# QuickNES (Nes_Emu), as the 3DS port uses it

The NES emulator behind the in-game NES games on 3DS (`game/nes_3ds.cpp`).

- **What:** Shay Green's Nes_Emu 0.7.0 ("QuickNES"), as maintained by libretro.
- **Where from:** <https://github.com/libretro/QuickNES_Core>, `nes_emu/` at commit
  `26bb785c9deddb66a17717b21bb4e328f03ade32` (2026-07-06).
- **Licence:** GNU Lesser General Public License 2.1 or later, as each `.cpp` file says.
  - Each header belongs to its `.cpp` file's module.
  - `abstract_file.cpp` has its own MIT-style licence, stated in the file.
  - `LICENSE` is the LGPL 2.1 text from gnu.org. The upstream repository's own `LICENSE` is the GPL 2,
    for libretro's frontend, which isn't here.
  - `tools/` holds upstream's generators for `blip_kernels.h` and `nes_nonlin_table.h` ("do not edit by
    hand"), the tables' source. They carry no licence of their own, so they fall under the upstream
    repository's GPL 2. They are not built: the program doesn't contain them.

## What is here

The emulator core and the mappers the port ships: 0 (NROM), 1 (MMC1), 2 (UxROM), 3 (CNROM), 4 (MMC3),
7 (AxROM), 9 (MMC2). The disc's 18 cartridge games use 0, 1, 4 and 9. Clu Clu Land D is a Disk System
image, which neither QuickNES nor the fixNES glue it replaced (iNES files only) runs.

Left out:
- libretro's frontend;
- the expansion sound chips (VRC6, VRC7, Namco 163, Sunsoft 5B), which no game here uses;
- the NTSC filter and the other mappers. Several of those are GPL or unlicensed contributions, not the
  library's LGPL.

## Changes from the upstream files

Every changed file says so near its top.

- `mappers/mapper009.hpp`: a new MMC2, written for the port from the NESdev wiki, under this library's
  licence. Upstream's file states no licence.
- `Nes_Mapper.cpp`: only the mappers above are listed, and they are made with `blargg_new`.
- `blargg_common.h`, `Nes_Core.cpp`, `Nes_Ppu_Impl.cpp`, `Nes_Emu.cpp`: objects and buffers come from
  `malloc`. The game replaces the global `operator new` with JSystem's heaps (`JKRHeap.cpp`), which are
  nearly full during an NES game. Only code the port never reaches still uses `new`/`delete`: save states,
  the effects buffer, `Auto_File_Reader`'s destructor.

Build settings (`platform/3ds/CMakeLists.txt`):
- `NO_UNALIGNED_ACCESS` declares the PPU's word accesses at odd addresses as unaligned. Without it, GCC
  may pair them into `LDRD`/`LDM`, which fault on the ARM11.
- Warnings are off for these files.

## For a release

The LGPL lets the port link this library statically as long as the people who get the binary also get
this library's source, with the changes, and the means to rebuild the program with a modified copy of it.
This repository is that source: a release should point to it and include `LICENSE`.
