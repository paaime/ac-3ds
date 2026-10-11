/* status_3ds.c - the bottom screen's console (see status_3ds.h). */
#include "status_3ds.h"

#include <3ds.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>

#include "audio_3ds.h"
#include "console_row.h"
#include "text_wrap.h"
#include "frame_3ds.h"
#include "heap_3ds.h"
#include "profiler_3ds.h"
#include "vi_3ds.h"

#define W                 CONSOLE_ROW_WIDTH_BOTTOM
#define MB                (1024.0 * 1024.0)
#define KB                1024u
#define REFRESH_FRAMES    20 /* 3 refreshes per second at 60 fps */
#define FIRST_LIVE_ROW    14
#define MAX_BOOT_ROWS     12
#define HINT_ROW          29
#define HINT              "Touch the screen: back to the map"
#define GAME_TITLE        "Animal Crossing"
#define TITLE_ROW         12
#define MESSAGE_ROW       4
#define MESSAGE_MARGIN    2  /* columns */
#define MESSAGE_LINES     18
#define ACTION_ROW        28
#define VERSION_ROW       30
#define COLOR_DIM         "\x1b[30;1m"
#define COLOR_TITLE       "\x1b[32;1m"
#define COLOR_ERROR       "\x1b[31;1m"
#define COLOR_WARNING     "\x1b[33;1m"
#define COLOR_TEXT        "\x1b[37m"
#define COLOR_RESET       "\x1b[0m"

static PrintConsole s_console;
static int s_boot_row;
static u64 s_last_refresh_tick;
static u32 s_frames_since_refresh;
static u32 s_peak_malloc;
static u32 s_peak_pool;
static bool s_console_on = true;
static bool s_visible = true;
static char s_boot_lines[MAX_BOOT_ROWS][CONSOLE_ROW_WIDTH_BOTTOM + 1]; /* to redraw after the map */
static char s_title[CONSOLE_ROW_WIDTH_BOTTOM + 1];

/* stdout goes to a null device, as the PC port sends it to NUL unless verbose;
 * the console is switched in only while this code draws. */
static const devoptab_t* s_console_dotab;

static ssize_t null_write(struct _reent* r, void* fd, const char* ptr, size_t len)
{
    (void)r;
    (void)fd;
    (void)ptr;
    return (ssize_t)len;
}

static const devoptab_t NULL_DOTAB = { .name = "null", .write_r = null_write };

static void console_begin(void)
{
    fflush(stdout);
    devoptab_list[STD_OUT] = s_console_dotab;
}

static void console_end(void)
{
    fflush(stdout);
    devoptab_list[STD_OUT] = &NULL_DOTAB;
}

void status3ds_console(bool on)
{
    s_console_on = on;
}

void status3ds_init(void)
{
    consoleInit(GFX_BOTTOM, &s_console);
    s_console_dotab = devoptab_list[STD_OUT];
    /* consoleInit takes stderr too; until open_log moves it, its "[boot]" lines
     * would print over the status rows. */
    devoptab_list[STD_ERR] = &NULL_DOTAB;
    snprintf(s_title, sizeof(s_title), "Animal Crossing 3DS %s (developer)", AC3DS_VERSION);
    if (AC3DS_DEBUG) {
        console_row_print(1, W, "%s", s_title);
    }
    s_boot_row = 2;
    console_end();
}

/* ---- boot screens (both builds) -------------------------------------------- */

/* Bypasses stdout, which verbose mode reopens on the log file. */
static void screen_write(const char* text)
{
    if (s_console_dotab != NULL && s_console_dotab->write_r != NULL) {
        s_console_dotab->write_r(_REENT, NULL, text, strlen(text));
    }
}

/* `text` at 1-based `row`, centred, in `color`. */
static void screen_centred(int row, const char* color, const char* text)
{
    char line[160];
    int len = (int)strnlen(text, W);
    snprintf(line, sizeof(line), "\x1b[%d;%dH%s%.*s" COLOR_RESET, row, text_center_column(len, W), color, len, text);
    screen_write(line);
}

static void screen_clear(void)
{
    screen_write(COLOR_RESET "\x1b[2J");
}

static void screen_message(const char* color, const char* heading, const char* message, const char* action)
{
    static char lines[TEXT_WRAP_MAX_LINES][TEXT_WRAP_MAX_WIDTH + 1];
    int n = text_wrap(message, W - 2 * MESSAGE_MARGIN, lines, MESSAGE_LINES);
    screen_clear();
    screen_centred(MESSAGE_ROW, color, heading);
    for (int i = 0; i < n; i++) {
        char line[96];
        snprintf(line, sizeof(line), "\x1b[%d;%dH%.*s", MESSAGE_ROW + 2 + i, MESSAGE_MARGIN + 1,
                 W - 2 * MESSAGE_MARGIN, lines[i]);
        screen_write(line);
    }
    screen_centred(ACTION_ROW, COLOR_TEXT, action);
    gfxFlushBuffers(); /* the console writes the framebuffer through the CPU cache */
}

/* The key pressed among `keys`; 0 if the system asks the game to close meanwhile
 * (HOME → Close, power off). */
static u32 wait_for(u32 keys)
{
    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown() & keys;
        if (down != 0) {
            return down;
        }
        gspWaitForVBlank();
    }
    return 0;
}

void status3ds_loading(const char* what)
{
    fprintf(stderr, "[boot] %s\n", what);
    if (AC3DS_DEBUG) {
        return; /* the developer build lists the boot steps instead */
    }
    screen_clear();
    screen_centred(TITLE_ROW, COLOR_TITLE, GAME_TITLE);
    screen_centred(TITLE_ROW + 2, COLOR_TEXT, what);
    screen_centred(VERSION_ROW, COLOR_DIM, "version " AC3DS_VERSION);
    gfxFlushBuffers();
}

void status3ds_fatal(const char* message)
{
    fprintf(stderr, "[boot] FATAL: %s\n", message);
    screen_message(COLOR_ERROR, GAME_TITLE " can't start", message, "Press START to exit.");
    wait_for(KEY_START);
}

bool status3ds_warning(const char* message)
{
    fprintf(stderr, "[boot] warning: %s\n", message);
    screen_message(COLOR_WARNING, "Before you play", message, "Press A to continue.");
    bool go_on = wait_for(KEY_A) != 0;
    screen_clear();
    gfxFlushBuffers();
    return go_on;
}

bool status3ds_ask(const char* heading, const char* message)
{
    fprintf(stderr, "[boot] asked: %s\n", message);
    screen_message(COLOR_WARNING, heading, message, "A: try anyway    START: quit");
    bool go_on = (wait_for(KEY_A | KEY_START) & KEY_A) != 0;
    fprintf(stderr, "[boot] %s\n", go_on ? "trying anyway" : "quit");
    screen_clear();
    gfxFlushBuffers();
    return go_on;
}

bool status3ds_console_available(void)
{
    return s_console_on;
}

void status3ds_set_visible(bool visible)
{
    s_visible = visible;
    if (!visible) {
        return;
    }
    s_last_refresh_tick = svcGetSystemTick(); /* the fps since now, not since the map opened */
    if (!s_console_on) { /* verbose mode: just clear the map away */
        u16 w, h;
        u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &w, &h);
        memset(fb, 0, (size_t)w * h * 2); /* RGB565 */
        gfxFlushBuffers();
        return;
    }
    console_begin();
    consoleClear();
    console_row_print(1, W, "%s", s_title);
    for (int i = 0; i < MAX_BOOT_ROWS && s_boot_lines[i][0] != '\0'; i++) {
        console_row_print(2 + i, W, "%s", s_boot_lines[i]);
    }
    console_row_print(HINT_ROW, W, "%s", HINT);
    console_end();
    s_frames_since_refresh = REFRESH_FRAMES;
}

void status3ds_step(const char* format, ...)
{
    char line[96];
    va_list args;
    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    fprintf(stderr, "[boot] %s\n", line);
    if (AC3DS_DEBUG && s_console_on && s_boot_row < 2 + MAX_BOOT_ROWS) {
        /* longer lines are cut on screen anyway */
        memcpy(s_boot_lines[s_boot_row - 2], line, sizeof(s_boot_lines[0]) - 1);
        s_boot_lines[s_boot_row - 2][sizeof(s_boot_lines[0]) - 1] = '\0';
        console_begin();
        console_row_print(s_boot_row++, W, "%s", line);
        console_end();
    }
}

/* Sampled on refresh frames only: mallinfo walks the whole free list. */
static void track_memory(void)
{
    struct mallinfo mi = mallinfo();
    u32 used = (u32)mi.uordblks;
    s_peak_malloc = used > s_peak_malloc ? used : s_peak_malloc;
    Heap3dsReport heap = heap3ds_report();
    u32 pool_used = heap.layout.pool_bytes - linearSpaceFree();
    s_peak_pool = pool_used > s_peak_pool ? pool_used : s_peak_pool;
}

static void draw_audio(int row)
{
    Audio3dsStatus a = audio3ds_status();
    if (!a.output_on) {
        console_row_print(row, W, "Audio off (see ac3ds.log)");
    } else if (a.mixer_core < 0) {
        console_row_print(row, W, "Audio game thread %lums lvl %lu%% under %lu", (unsigned long)a.queued_ms,
                          (unsigned long)a.level_percent, (unsigned long)a.underruns);
    } else {
        console_row_print(row, W, "Audio core %d %lums lvl %lu%% under %lu", a.mixer_core,
                          (unsigned long)a.queued_ms, (unsigned long)a.level_percent,
                          (unsigned long)a.underruns);
    }
}

static void draw_profile(int row)
{
    Prof3dsReport p = prof3ds_report();
    if (p.frames == 0) {
        return;
    }
    console_row_print(row, W, "CPU logic %4.1f emu64 %4.1f dl %3.1f", p.ms[PROF_LOGIC], p.ms[PROF_EMU64],
                      p.ms[PROF_DRAW_FINISH]);
    console_row_print(row + 1, W, "wait %4.1f end %3.1f flush %3.1f tex %3.1f", p.ms[PROF_FRAME_WAIT],
                      p.ms[PROF_FRAME_END], p.ms[PROF_FLUSH], p.ms[PROF_TEXTURES]);
    console_row_print(row + 2, W, "status %3.1f snd %3.1f mix2 %2.0f%% %4.1fms", p.ms[PROF_STATUS],
                      p.ms[PROF_AUDIO_GAME], p.mixer_percent, p.frame_ms);
}

static void draw_live(void)
{
    Frame3dsStats f = frame3ds_stats();
    u64 now = svcGetSystemTick();
    double ms = (double)(now - s_last_refresh_tick) / CPU_TICKS_PER_MSEC;
    double fps = ms > 0.0 ? s_frames_since_refresh * 1000.0 / ms : 0.0;
    Heap3dsReport heap = heap3ds_report();
    int r = FIRST_LIVE_ROW;
    console_row_print(r++, W, "Frame %lu  %.1f fps  idle %lu", (unsigned long)f.frames, fps,
                      (unsigned long)f.idle_waits);
    console_row_print(r++, W, "citro3d CPU %5.2f GPU %5.2f ms", f.cpu_ms, f.gpu_ms);
    console_row_print(r++, W, "Draws %lu  vtx %lu  dropped %lu", (unsigned long)f.gx.draws,
                      (unsigned long)f.gx.vertices, (unsigned long)f.gx.dropped_draws);
    console_row_print(r++, W, "CmdBuf %4.1f%% words/draw %lu max %lu", f.cmdbuf_usage * 100.0f,
                      (unsigned long)(f.gx.draws ? f.gx.cmd_words / f.gx.draws : 0),
                      (unsigned long)f.gx.max_draw_cmd_words);
    console_row_print(r++, W, "Tex %lu (%lu KB) new %lu fail %lu", (unsigned long)f.gx.texture_entries,
                      (unsigned long)(f.gx.texture_bytes / KB), (unsigned long)f.gx.texture_uploads,
                      (unsigned long)f.gx.texture_failures);
    console_row_print(r++, W, "TEV fb %lu skip %lu fog %lu efb %lu", (unsigned long)f.gx.tev_fallbacks,
                      (unsigned long)f.gx.skipped_primitives, (unsigned long)f.gx.fog_tables,
                      (unsigned long)f.gx.efb_copy_draws);
    Emu64FrameCounts e = vi3ds_last_emu64_counts();
    console_row_print(r++, W, "emu64 cmd %d tri %d vtx %d dl %d", e.cmds, e.tri_cmds, e.vtx_cmds, e.dl_cmds);
    console_row_print(r++, W, "malloc %.1f / %.1f MB (peak %.1f)", mallinfo().uordblks / MB,
                      heap.layout.malloc_bytes / MB, s_peak_malloc / MB);
    console_row_print(r++, W, "pool %.1f / %.1f MB (peak %.1f)",
                      (heap.layout.pool_bytes - linearSpaceFree()) / MB, heap.layout.pool_bytes / MB,
                      s_peak_pool / MB);
    draw_audio(r++);
    draw_profile(r);
    s_last_refresh_tick = now;
    s_frames_since_refresh = 0;
}

void status3ds_frame(void)
{
    s_frames_since_refresh++;
    if (s_frames_since_refresh < REFRESH_FRAMES) {
        return;
    }
    track_memory();
    if (!s_console_on || !s_visible) {
        s_frames_since_refresh = 0;
    } else {
        console_begin();
        draw_live();
        console_end();
        gfxFlushBuffers();
    }
}

void status3ds_log_summary(void)
{
    Frame3dsStats f = frame3ds_stats();
    fprintf(stderr, "[exit] frames %lu, peak malloc %.1f MB, peak pool %.1f MB\n", (unsigned long)f.frames,
            s_peak_malloc / MB, s_peak_pool / MB);
}
