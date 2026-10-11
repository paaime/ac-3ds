# Game sources for the 3DS build: a verbatim copy of the source selection in
# pc/CMakeLists.txt (commit 5079eef, lines 141-263 and 356-371), so the 3DS
# build compiles exactly the files the PC port does. Regenerate with
# platform/3ds/tools/sync_game_sources.py; don't edit by hand.
#
# Expects DECOMP_ROOT. Defines GAME_C_SOURCES and GAME_CXX_SOURCES and sets the
# C++ language on the .c files that contain C++.

# ---- copied from pc/CMakeLists.txt ----
# Collect game source files
# =============================================================================
file(GLOB_RECURSE GAME_C_SOURCES CONFIGURE_DEPENDS
    "${DECOMP_ROOT}/src/*.c"
)
file(GLOB_RECURSE GAME_CXX_SOURCES CONFIGURE_DEPENDS
    "${DECOMP_ROOT}/src/*.cpp"
)

# Exclude files that conflict with PC port or are platform-specific
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/AUS/.*")  # AUS-specific versions

# Exclude all Dolphin SDK hardware abstraction (we replace with pc_*.c stubs)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/gx/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/os/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/vi/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/pad/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/card/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/dvd/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/ai/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/ar/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/dsp/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/exi/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/si/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/db/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/mtx/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/base/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/gba/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/amcstubs/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/OdemuExi2/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/dolphin/odenotstub/.*")

# Exclude PPC-specific runtime/compiler support (Metrowerks CodeWarrior)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/TRK_MINNOW_DOLPHIN/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/Runtime\\.PPCEABI\\.H/.*")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/MSL_C\\.PPCEABI\\.bare\\.H/.*")

# Bootdata: Include window/logo model display lists (gam_win1.c, gam_win2.c, gam_win3.c, logo_nin.c)
# These provide Gfx[] arrays used by initial_menu.c for UI windows and Nintendo logo.

# Include ALL data files now that .inc assets are extracted from ROM
# (build/GAFE01_00/include/ is in the include path)
file(GLOB_RECURSE DATA_C_SOURCES CONFIGURE_DEPENDS "${DECOMP_ROOT}/src/data/*.c")
list(APPEND GAME_C_SOURCES ${DATA_C_SOURCES})

# f_furniture.c #includes all 696 src/furniture/*.c files as a single TU (like GC build).
# Exclude the individual files to avoid duplicate symbol definitions.
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/src/furniture/.*")

# Exclude duplicate data files (subdirectory verbose versions conflict with top-level)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/src/data/model/obj_e_boat/.*")

# Exclude template/include .c files that are #included from other files
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/ac_npc_shop_common\\.c$")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/ac_animal_logo_misc\\.c$")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/m_item_debug\\.c$")

# Exclude NPC anime data fragments (no headers, #included from other NPC files)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/actor/npc/.*_anime\\.c$")

# Exclude Dolphin OS sources that conflict with PC stubs
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/osreport\\.c$")
# audio.c now included (jaudio_NES provides Na_* functions)

# Exclude Famicom/NES emulator (contains PPC inline assembly)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/Famicom/.*")

# Exclude MSL_C library sources (using system libc on PC)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/MSL_C/.*")

# jaudio_NES: Include audio engine, exclude GBA-specific files
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/jaudio_NES/internal/dsp_cardunlock\\.c$")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/jaudio_NES/internal/dsp_GBAKey\\.c$")

# libc64: Include most files (qrand, math64, sleep, etc.)
# Exclude malloc.c (redefines system malloc/free which crashes CRT on PC)
# Keep __osMalloc.c (game code uses __osMalloc* directly via m_malloc.c)
# Exclude sprintf.c/aprintf.c (they need _Printf from libultra xprintf)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/libc64/malloc\\.c$")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/libc64/sprintf\\.c$")
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/libc64/aprintf\\.c$")

# libultra: Exclude most (we have pc_os.c replacements), but keep xprintf for libc64
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/libultra/.*")

# Exclude PPC-specific files
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/ReconfigBATs\\.c$")

# Exclude GC-specific entry point (PC has its own main)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/executor\\.c$")

# ROM asset .inc files now available in build/GAFE01_00/include/assets/
# All previously-excluded files are now compiled with real data.

# Exclude ef_effect_lib.c (has no #includes, pure function definitions without type context)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/ef_effect_lib\\.c$")

# Exclude m_card.c (needs Dolphin CARD system headers with non-standard include paths)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/m_card\\.c$")

# Also filter C++ sources
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/AUS/.*")
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/dolphin/.*")
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/TRK_MINNOW_DOLPHIN/.*")
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/Runtime\\.PPCEABI\\.H/.*")
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/MSL_C\\.PPCEABI\\.bare\\.H/.*")
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/Famicom/.*")

# Re-include Famicom files (PPC asm guarded out with #ifndef TARGET_PC)
list(APPEND GAME_CXX_SOURCES
    ${DECOMP_ROOT}/src/static/Famicom/famicom.cpp
    ${DECOMP_ROOT}/src/static/Famicom/famicom_nesinfo.cpp
    ${DECOMP_ROOT}/src/static/Famicom/ks_nes_core.cpp
    ${DECOMP_ROOT}/src/static/Famicom/ks_nes_draw.cpp
)

# Exclude emu64_print.cpp (it's #included directly from emu64.c, not a standalone file)
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/emu64_print\\.cpp$")

# libjsys: jsyswrapper_ext.cpp and jsyswrapper_main.cpp are #included from jsyswrapper.cpp
# so exclude them from standalone compilation (just like emu64_utility.c)
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/jsyswrapper_ext\\.cpp$")
list(FILTER GAME_CXX_SOURCES EXCLUDE REGEX ".*/jsyswrapper_main\\.cpp$")

# Exclude emu64_utility.c (it's #included from emu64.c, not a standalone file)
list(FILTER GAME_C_SOURCES EXCLUDE REGEX ".*/emu64_utility\\.c$")

# These .c files contain C++ code (classes, namespaces, ::) - compile as C++
# Only files that truly need C++ (std::, extern "C", etc.) - NOT just for nullptr
set_source_files_properties(
    "${DECOMP_ROOT}/src/static/libforest/emu64/emu64.c"
    # ja_calc.c uses std::sinf, std::sqrtf
    "${DECOMP_ROOT}/src/static/jaudio_NES/internal/ja_calc.c"
    # sample.c: reinterpret_cast replaced with C cast under TARGET_PC, no longer needs C++
    # jammain_2.c uses extern "C"
    "${DECOMP_ROOT}/src/static/jaudio_NES/internal/jammain_2.c"
    # game64.c includes .c_inc with extern "C" (OSAttention.c_inc)
    "${DECOMP_ROOT}/src/static/jaudio_NES/game/game64.c"
    PROPERTIES LANGUAGE CXX
)
# ---- end of copy ----
