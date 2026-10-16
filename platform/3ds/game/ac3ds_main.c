#include "pc_platform.h"

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "apt_trace_3ds.h"
#include "backlight_3ds.h"
#include "disc_3ds.h"
#include "dsp_firmware_3ds.h"
#include "frame_3ds.h"
#include "heap_3ds.h"
#include "pad_3ds.h"
#include "settings_3ds.h"
#include "status_3ds.h"
#include "test_flags_3ds.h"
#include "trace_3ds.h"

#define DATA_DIR     "sdmc:/3ds/ac3ds"
#define LOG_FILE     "ac3ds.log"
#define MB           (1024u * 1024u)

u32 __stacksize__ = 1024 * 1024;

int g_pc_running = 1;
int g_pc_verbose = 0;
int g_pc_frame_limit_override = -1;
int g_pc_speedhack_enabled = 0;
int g_pc_time_override = -1;
int g_pc_min_override = -1;
int g_pc_sec_override = -1;
int g_pc_date_month = -1;
int g_pc_date_day = -1;
int g_pc_date_year = -1;
int g_pc_weather_override = -1;
int g_pc_weather_intensity_override = 2; /* mEnv_WEATHER_INTENSITY_HEAVY */
int g_pc_window_w = PC_SCREEN_WIDTH;
int g_pc_window_h = PC_SCREEN_HEIGHT;
int g_pc_widescreen_stretch = 0;

int pc_emu64_frame_cmds;
int pc_emu64_frame_noop_cmds;
int pc_emu64_frame_tri_cmds;
int pc_emu64_frame_vtx_cmds;
int pc_emu64_frame_dl_cmds;
int pc_emu64_frame_cull_visible;
int pc_emu64_frame_cull_rejected;

/* emu64's seg2k0 range: the .3dsx load address (3dsx.ld) to __end__. */
#define IMAGE_BASE 0x00100000u
extern char __end__[];
unsigned int pc_image_base = IMAGE_BASE;
unsigned int pc_image_end;

void pc_platform_init(void) {}

void pc_platform_update_window_size(void) {}

void pc_platform_swap_buffers(void)
{
    status3ds_frame();
}

int pc_platform_poll_events(void)
{
    if (!aptMainLoop()) {
        return 0;
    }
    hidScanInput();
    pad3ds_hotkeys();
    return 1;
}

void pc_disc_shutdown(void);

static bool s_game_started;
static bool s_shut_down;

/* Also called by main.c's mainproc before exit(0); the mixer stops before its message queues. */
void pc_platform_shutdown(void)
{
    if (s_shut_down) {
        return;
    }
    s_shut_down = true;
    backlight3ds_exit(); /* first, so a hang below can't leave the bottom screen dark */
    trace3ds_stop();
    if (s_game_started) {
        status3ds_log_summary();
    }
    pc_audio_shutdown();
    pc_audio_mq_shutdown();
    pc_disc_shutdown();
    frame3ds_shutdown();
    fflush(stderr);
    gfxExit();
}

int pc_assets_init(void);

/* The game's main() renamed to ac_entry, boot.c's to boot_main (CMakeLists.txt). */
extern void ac_entry(void);
extern int boot_main(int argc, const char** argv);

static void open_log(void)
{
    FILE* probe = fopen(LOG_FILE, "w");
    if (probe == NULL) {
        consoleDebugInit(debugDevice_SVC);
        return;
    }
    fclose(probe);
    if (freopen(LOG_FILE, "a", stderr) == NULL) {
        return;
    }
    setvbuf(stderr, NULL, _IOLBF, 0);
    if (settings3ds_on(SETTING_VERBOSE_LOG) && freopen(LOG_FILE, "a", stdout) != NULL) {
        g_pc_verbose = 1;
        status3ds_console(false);
        setvbuf(stdout, NULL, _IOLBF, 0);
    }
}

static void report_memory(void)
{
    Heap3dsReport h = heap3ds_report();
    status3ds_step("Memory: malloc %lu MB, pool %lu MB", (unsigned long)(h.layout.malloc_bytes / MB),
                   (unsigned long)(h.layout.pool_bytes / MB));
    status3ds_step("  %lu MB free, image end %08X", (unsigned long)(h.available_bytes / MB), pc_image_end);
    if (h.shrink_steps != 0) {
        status3ds_step("  (linear block shrunk %lu times)", (unsigned long)h.shrink_steps);
    }
    if (h.layout.tight) {
        status3ds_step("  under the usual %lu MB (original 3DS)", (unsigned long)(h.needed_bytes / MB));
    }
}

static bool is_new_3ds(void)
{
    bool is_new = false;
    if (R_FAILED(APT_CheckNew3DS(&is_new))) {
        is_new = true;
    }
    return is_new;
}

static bool supported_console(bool is_new)
{
    if (is_new) {
        return true;
    }
    return status3ds_ask("An original 3DS or 2DS",
                         "This game was made for the New Nintendo 3DS and hasn't been tested on the original "
                         "3DS or 2DS.");
}

static bool enough_memory(bool is_new)
{
    Heap3dsReport h = heap3ds_report();
    if (!h.rescue && (!h.layout.tight || !is_new)) {
        return true;
    }
    char message[320];
    snprintf(message, sizeof(message), "Not enough memory: %lu MB free, %lu MB needed.\n\n%s",
             (unsigned long)(h.available_bytes / MB),
             (unsigned long)((is_new ? h.needed_bytes : h.tight_needed_bytes) / MB),
             is_new ? "Start the Homebrew Launcher from Download Play (or another game or app), then start "
                      "Animal Crossing again."
                    : "On an original 3DS or 2DS, install the .cia version: it asks the system for more "
                      "memory than the Homebrew Launcher can give.");
    status3ds_fatal(message);
    return false;
}

static bool enter_data_folder(void)
{
    if (chdir(DATA_DIR) == 0) {
        return true;
    }
    mkdir("sdmc:/3ds", 0777);
    mkdir(DATA_DIR, 0777);
    if (chdir(DATA_DIR) == 0) {
        return true;
    }
    status3ds_fatal("The folder " DATA_DIR "/ couldn't be opened or made on the SD card.\n\n"
                    "Check that the SD card isn't locked or full.");
    return false;
}

static bool check_sound_firmware(void)
{
    DspFirmware fw = dsp_firmware_load();
    bool go_on = true;
    if (fw.data == NULL) {
        go_on = status3ds_warning("There will be no sound: the 3DS's sound firmware isn't on the SD card.\n\n"
                          "To add it, open the Rosalina menu (L + Down + SELECT), then Miscellaneous "
                          "options, then Dump DSP firmware, and start the game again.");
    }
    free(fw.data);
    return go_on;
}

static bool load_game(void)
{
    status3ds_loading("Starting the graphics...");
    if (!frame3ds_init()) {
        status3ds_fatal("The graphics couldn't start.\n\nRestart the console and try again.");
        return false;
    }
    status3ds_step("Graphics ready");
    status3ds_loading("Reading your disc image...");
    if (!disc3ds_open()) {
        return false;
    }
    status3ds_step("Disc image open");
    status3ds_loading("Loading the game...");
    if (!pc_assets_init()) {
        char message[320];
        snprintf(message, sizeof(message),
                 "The game's data couldn't be read from %.120s.\n\n"
                 "The file may be incomplete or damaged: copy it to the SD card again, or dump your disc again.",
                 disc3ds_name());
        status3ds_fatal(message);
        return false;
    }
    status3ds_step("Assets loaded; starting the game");
    if (!check_sound_firmware()) {
        return false;
    }
    status3ds_loading("Starting the game...");
    return true;
}

static bool boot(void)
{
    bool is_new = is_new_3ds();
    if (!supported_console(is_new) || !enough_memory(is_new) || !enter_data_folder()) {
        return false;
    }
    settings3ds_load();
    open_log();
    fprintf(stderr, "[boot] Animal Crossing 3DS %s, %s build\n", AC3DS_VERSION, AC3DS_DEBUG ? "developer" : "player");
    settings3ds_log();
    trace3ds_start(settings3ds_on(SETTING_VERBOSE_LOG));
    apt_trace3ds_before_libs(settings3ds_on(SETTING_VERBOSE_LOG));
    test_flags_apply();
    report_memory();
    return load_game();
}

int main(int argc, char** argv)
{
    pc_image_end = (unsigned int)(uintptr_t)__end__;
    osSetSpeedupEnable(true);
    gfxInitDefault();
    backlight3ds_init();
    status3ds_init();
    status3ds_loading("Loading...");

    if (boot()) {
        s_game_started = true;
        trace3ds_mark("boot: the game's own start-up (ac_entry, boot_main)");
        ac_entry();
        boot_main(argc, (const char**)argv);
    }
    pc_platform_shutdown();
    return 0;
}
