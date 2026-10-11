# Animal Crossing (GAFE01) for New 3DS: the whole game. Configure from the repo root with
# -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake; platform/3ds/README.md has the rest.
cmake_minimum_required(VERSION 3.16)
project(ac3ds VERSION 0.1.0 LANGUAGES C CXX ASM)

if(NOT NINTENDO_3DS)
    message(FATAL_ERROR "Configure with -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS ON)

get_filename_component(DECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(P3DS "${CMAKE_CURRENT_SOURCE_DIR}")
set(PC_DIR "${DECOMP_ROOT}/pc")

# =============================================================================
# Sources
# =============================================================================
include(cmake/game_sources.cmake)

# PC port platform files with no SDL/OpenGL, reused unchanged.
set(PC_REUSED_SOURCES
    ${PC_DIR}/src/pc_aram.c
    ${PC_DIR}/src/pc_assets.c
    ${PC_DIR}/src/pc_card.c
    ${PC_DIR}/src/pc_disc.c
    ${PC_DIR}/src/pc_dvd.c
    ${PC_DIR}/src/pc_gbi_runtime.c
    ${PC_DIR}/src/pc_m_card.c
    ${PC_DIR}/src/pc_menu_util.c
    ${PC_DIR}/src/pc_misc.c
    ${PC_DIR}/src/pc_mtx.c
    ${PC_DIR}/src/pc_save_bswap.c
    ${PC_DIR}/src/pc_stubs.c
    ${PC_DIR}/src/pc_stubs_cpp.cpp
    ${PC_DIR}/src/pc_text_draw.c
    ${DECOMP_ROOT}/src/static/dolphin/pad/Padclamp.c
    # libultra's sins/coss, the GameCube game's own: a 1024-entry table. The PC
    # port leaves libultra out and computes them in double precision
    # (pc_misc.c, renamed away below), which the 3DS's CPU pays for.
    ${DECOMP_ROOT}/src/static/libultra/gu/sins.c
    ${DECOMP_ROOT}/src/static/libultra/gu/coss.c
)

# 3DS platform layer: replacements for the SDL-based PC files, the GX backend
# and the shared modules.
file(GLOB P3DS_GAME_SOURCES CONFIGURE_DEPENDS "${P3DS}/game/*.c" "${P3DS}/game/*.cpp")
# The NES glue includes QuickNES's headers, not the game's.
set(P3DS_NES_GLUE ${P3DS}/game/nes_3ds.cpp)
list(REMOVE_ITEM P3DS_GAME_SOURCES ${P3DS_NES_GLUE})

# QuickNES (Shay Green, LGPL 2.1+), the NES games' emulator: the part of
# libretro's QuickNES_Core the port uses (lib/quicknes/README.md).
set(QUICKNES_DIR ${P3DS}/lib/quicknes)
file(GLOB QUICKNES_SOURCES CONFIGURE_DEPENDS ${QUICKNES_DIR}/*.cpp)
set(P3DS_GFX_SOURCES
    ${P3DS}/gfx/gx_alpha_test.c
    ${P3DS}/gfx/gx_api.c
    ${P3DS}/gfx/gx_api_frame.c
    ${P3DS}/gfx/gx_bind.c
    ${P3DS}/gfx/gx_debug.c
    ${P3DS}/gfx/gx_dlist.c
    ${P3DS}/gfx/gx_efb.c
    ${P3DS}/gfx/gx_fog.c
    ${P3DS}/gfx/gx_fog_apply.c
    ${P3DS}/gfx/gx_gpu_check.c
    ${P3DS}/gfx/gx_pixel.c
    ${P3DS}/gfx/gx_render.c
    ${P3DS}/gfx/gx_screen.c
    ${P3DS}/gfx/gx_tev.c
    ${P3DS}/gfx/gx_tev_cache.c
    ${P3DS}/gfx/gx_texcache.c
    ${P3DS}/gfx/gx_uniforms.c
    ${P3DS}/gfx/gx_uv.c
)
set(P3DS_COMMON_SOURCES
    ${P3DS}/common/acre_select.c
    ${P3DS}/common/acre_sync.c
    ${P3DS}/common/audio_pacer.c
    ${P3DS}/common/bottom_layout.c
    ${P3DS}/common/console_row.c
    ${P3DS}/common/dsp_firmware.c
    ${P3DS}/common/feed_rate.c
    ${P3DS}/common/gc_clock.c
    ${P3DS}/common/gc_ortho.c
    ${P3DS}/common/gc_pattern.c
    ${P3DS}/common/gc_texture.c
    ${P3DS}/common/debounce.c
    ${P3DS}/common/disc_check.c
    ${P3DS}/common/light_timer.c
    ${P3DS}/common/mem_layout.c
    ${P3DS}/common/nes_pixels.c
    ${P3DS}/common/pad_map.c
    ${P3DS}/common/pcm_level.c
    ${P3DS}/common/pcm_stereo.c
    ${P3DS}/common/pica_tile.c
    ${P3DS}/common/pot_pad.c
    ${P3DS}/common/press_hold.c
    ${P3DS}/common/prof_window.c
    ${P3DS}/common/quest_bits.c
    ${P3DS}/common/quick_save.c
    ${P3DS}/common/settings_file.c
    ${P3DS}/common/stall_watch.c
    ${P3DS}/common/tex_memo.c
    ${P3DS}/common/text_wrap.c
    ${P3DS}/common/tga.c
    ${P3DS}/common/wave_pool.c
)

# =============================================================================
# Compile settings (as pc/CMakeLists.txt, plus TARGET_3DS)
# =============================================================================
# OFF (the default) is the player build; ON, in a build folder of its own, also adds nes_test.
option(AC3DS_DEBUG "Developer build: Testing settings, debug stats, SELECT hotkeys" OFF)
# KEYBOARD_TYPING is left out: it's a PC-only feature.
add_compile_definitions(
    TARGET_PC
    TARGET_3DS
    VERSION=0
    F3DEX_GBI_2
    NDEBUG
    BUGFIXES
    _LANGUAGE_C
    PC_ENHANCEMENTS
    AC3DS_DEBUG=$<BOOL:${AC3DS_DEBUG}>
)
add_compile_options(-O2 -fno-strict-aliasing -fwrapv)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Dnullptr=NULL>)

# platform/3ds/include first, so its pc_platform.h wins; it is also
# force-included into the game and PC files. The decomp and PC-port
# headers are system headers: their warnings aren't ours.
include_directories(${P3DS}/include)
include_directories(SYSTEM
    ${DECOMP_ROOT}/include
    ${DECOMP_ROOT}/src
    ${DECOMP_ROOT}
    ${PC_DIR}/include
)
set(PLATFORM_SHIM "${P3DS}/include/pc_platform.h")

# Decomp code: no warnings and -fpermissive, as on PC (GCC 14+ makes implicit
# declarations, implicit int and pointer/int mismatches errors otherwise).
# char signedness per library, as the original build: game code unsigned
# (ARM's default), Dolphin SDK, JSystem, jaudio internals and MSL signed.
set(DECOMP_FLAGS "-w;-fpermissive;-include;${PLATFORM_SHIM}")
set_source_files_properties(${GAME_C_SOURCES} ${GAME_CXX_SOURCES} ${PC_REUSED_SOURCES}
    PROPERTIES COMPILE_OPTIONS "${DECOMP_FLAGS}")
set(SIGNED_CHAR_SOURCES ${GAME_C_SOURCES} ${GAME_CXX_SOURCES})
list(FILTER SIGNED_CHAR_SOURCES INCLUDE REGEX
     ".*/src/static/(dolphin|JSystem|jaudio_NES/internal|MSL_C)/.*")
set_property(SOURCE ${SIGNED_CHAR_SOURCES} APPEND PROPERTY COMPILE_OPTIONS "-fsigned-char")
# The one game file that includes <SDL.h> gets a minimal libctru version of it.
set_property(SOURCE "${DECOMP_ROOT}/src/static/jaudio_NES/internal/os.c"
             APPEND PROPERTY INCLUDE_DIRECTORIES "${P3DS}/include/sdl_compat")

# pc_stubs.c's sound-mode stubs answer mono; the 3DS versions (game/audio_3ds.c)
# replace them, so the copies in that file are renamed away.
set_property(SOURCE "${PC_DIR}/src/pc_stubs.c" APPEND PROPERTY COMPILE_DEFINITIONS
             "OSGetSoundMode=pc_stub_OSGetSoundMode;OSSetSoundMode=pc_stub_OSSetSoundMode")

# pc_misc.c's double-precision sins/coss give way to libultra's (above).
set_property(SOURCE "${PC_DIR}/src/pc_misc.c" APPEND PROPERTY COMPILE_DEFINITIONS
             "sins=pc_misc_sins;coss=pc_misc_coss")

# pc_mtx.c's C_MTXOrtho maps depth to [0, 1]; the SDK's (game/mtx_3ds.c) maps
# it to GX's [-1, 0], which the PICA clips to.
set_property(SOURCE "${PC_DIR}/src/pc_mtx.c" APPEND PROPERTY COMPILE_DEFINITIONS
             "C_MTXOrtho=pc_C_MTXOrtho")

# The save code's files are flushed to the SD card before they're closed and
# renamed (game/save_3ds.c).
set_property(SOURCE "${PC_DIR}/src/pc_m_card.c" "${PC_DIR}/src/pc_card.c" APPEND PROPERTY COMPILE_DEFINITIONS
             "fclose=ac3ds_fclose_durable")

# Disc image reads are counted for the watchdog's reports (game/trace_3ds.c).
set_property(SOURCE "${PC_DIR}/src/pc_disc.c" APPEND PROPERTY COMPILE_DEFINITIONS "fread=ac3ds_disc_fread"
             "readdir=ac3ds_disc_readdir") # hidden entries skipped: macOS "._" files (game/disc_3ds.c)

# The release's version, where it's shown (the log, the loading screen).
set_property(SOURCE "${P3DS}/game/ac3ds_main.c" "${P3DS}/game/status_3ds.c" APPEND PROPERTY COMPILE_DEFINITIONS
             "AC3DS_VERSION=\"${PROJECT_VERSION}\"")

# Renamed entry points, as on PC: the 3DS main() (game/ac3ds_main.c) calls them.
set_property(SOURCE "${DECOMP_ROOT}/src/main.c" APPEND PROPERTY COMPILE_DEFINITIONS "main=ac_entry")
set_property(SOURCE "${DECOMP_ROOT}/src/static/boot.c" APPEND PROPERTY COMPILE_DEFINITIONS "main=boot_main")

set(P3DS_SOURCES ${P3DS_GAME_SOURCES} ${P3DS_GFX_SOURCES} ${P3DS_COMMON_SOURCES})
set_source_files_properties(${P3DS_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra")
set_property(SOURCE ${P3DS_SOURCES} APPEND PROPERTY INCLUDE_DIRECTORIES
             "${P3DS}/common" "${P3DS}/gfx" "${P3DS}/game" "${P3DS}/include")
# os_3ds.c is a copy of pc/src/pc_os.c: it keeps the PC layer's warning set
# (pc/CMakeLists.txt PC_WARN_SUPPRESS).
set_property(SOURCE "${P3DS}/game/os_3ds.c" APPEND PROPERTY COMPILE_OPTIONS
             "-Wno-unused-parameter;-Wno-sign-compare")
# QuickNES: third-party code, its warnings aren't ours. Its PPU reads and
# writes 32-bit words at odd addresses: NO_UNALIGNED_ACCESS declares them so,
# or GCC may pair them into LDRD/LDM, which fault on the ARM11 when unaligned.
set_source_files_properties(${QUICKNES_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-w"
    COMPILE_DEFINITIONS "NO_UNALIGNED_ACCESS"
    INCLUDE_DIRECTORIES "${QUICKNES_DIR}")
set_source_files_properties(${P3DS_NES_GLUE} PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra"
    COMPILE_DEFINITIONS "NO_UNALIGNED_ACCESS"
    INCLUDE_DIRECTORIES "${P3DS}/common;${P3DS}/gfx;${P3DS}/game")

# These call into the game: decomp headers declare static functions they
# never define.
set_property(SOURCE "${P3DS}/game/test_flags_3ds.c" "${P3DS}/game/town_map_3ds.c" "${P3DS}/game/bottom_dl_3ds.c"
             "${P3DS}/game/bottom_ui_3ds.c" "${P3DS}/game/shop_3ds.c" "${P3DS}/game/acres_3ds.c" "${P3DS}/game/quick_save_3ds.c"
             "${P3DS}/game/weeds_3ds.c" APPEND PROPERTY COMPILE_OPTIONS "-Wno-unused-function")

# Files that include decomp headers also get the platform shim.
set_property(SOURCE ${P3DS_GAME_SOURCES} ${P3DS}/gfx/gx_api.c ${P3DS}/gfx/gx_api_frame.c
             APPEND PROPERTY COMPILE_OPTIONS "-include;${PLATFORM_SHIM}")

# =============================================================================
# Target
# =============================================================================
ctr_add_shader_library(gx ${P3DS}/gfx/gx.v.pica)   # -> gx.shbin, header gx_shbin.h
dkp_add_embedded_binary_library(gx_shader gx)

add_executable(ac3ds
    ${GAME_C_SOURCES}
    ${GAME_CXX_SOURCES}
    ${PC_REUSED_SOURCES}
    ${P3DS_SOURCES}
    ${P3DS_NES_GLUE}
    ${QUICKNES_SOURCES}
)
target_link_libraries(ac3ds PRIVATE gx_shader citro3d)
target_link_options(ac3ds PRIVATE "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/ac3ds.map")

if(AC3DS_DEBUG)
    set(AC3DS_TITLE "Animal Crossing 3DS (developer)")
else()
    set(AC3DS_TITLE "Animal Crossing 3DS")
endif()
ctr_generate_smdh(ac3ds.smdh
    NAME "${AC3DS_TITLE}"
    DESCRIPTION "${AC3DS_TITLE} ${PROJECT_VERSION}."
    AUTHOR "paaime"
    ICON "${P3DS}/res/icon.png"
)
ctr_create_3dsx(ac3ds SMDH ac3ds.smdh)
