#include <3ds.h>
#include <stdio.h>

#include "stall_watch.h"
#include "trace_3ds.h"
#include "clock_3ds.h"

#define WATCH_CORE      2
#define WATCH_STACK     (16u * 1024u)
#define WATCH_PRIO      0x3F /* the lowest */
#define WATCH_PERIOD_NS 500000000LL
#define STALL_MS        4000u
#define REPEAT_MS       15000u
#define LOGGED_FRAMES   3
#define STATS_FRAMES    40

/* The watchdog reads the marks without a lock: a torn read only shows the previous step's name. */
static volatile const char* s_stage = "boot";
static volatile uint32_t s_seq;
static volatile uint32_t s_frame;
static volatile bool s_stop;
static bool s_detailed;
static volatile uint32_t s_disc_reads;
static volatile uint64_t s_disc_bytes;
static uint64_t s_disc_bytes_reported;
static Thread s_thread;

void trace3ds_mark(const char* stage)
{
    s_stage = stage;
    s_seq = s_seq + 1;
    if (s_detailed && s_frame <= LOGGED_FRAMES) {
        fprintf(stderr, "[trace] frame %lu: %s\n", (unsigned long)s_frame, stage);
    }
}

void trace3ds_frame(uint32_t frame)
{
    s_frame = frame;
}

void trace3ds_frame_stats(TraceFrameStats s)
{
    if (!s_detailed || s_frame > STATS_FRAMES) {
        return;
    }
    fprintf(stderr,
            "[trace] frame %lu holds: %lu draws, %lu vertices, %lu new textures (%lu failed), %lu EFB copies so far "
            "(%lu draws use one), %lu fog tables, %lu cmd words (max %lu per draw), %lu dropped, bottom %s\n",
            (unsigned long)s_frame, (unsigned long)s.draws, (unsigned long)s.vertices, (unsigned long)s.texture_uploads,
            (unsigned long)s.texture_failures, (unsigned long)s.efb_copies, (unsigned long)s.efb_copy_draws,
            (unsigned long)s.fog_tables, (unsigned long)s.cmd_words, (unsigned long)s.max_draw_cmd_words,
            (unsigned long)s.dropped_draws, s.bottom_drawn ? "redrawn" : "kept");
}

size_t ac3ds_disc_fread(void* ptr, size_t size, size_t count, FILE* file)
{
    size_t n = fread(ptr, size, count, file);
    s_disc_reads = s_disc_reads + 1;
    s_disc_bytes = s_disc_bytes + (uint64_t)n * size;
    return n;
}

static void report_disc(void)
{
    uint64_t bytes = s_disc_bytes;
    fprintf(stderr, "[watchdog]   disc image: %lu reads, %.1f MB in all, %.2f MB since the last report\n",
            (unsigned long)s_disc_reads, bytes / (1024.0 * 1024.0),
            (bytes - s_disc_bytes_reported) / (1024.0 * 1024.0));
    /* In a HOME hang: active is stuck before the jump, inactive is waiting for the HOME Menu. */
    fprintf(stderr, "[watchdog]   APT: the game is %s\n", aptIsActive() ? "active" : "inactive (handed over)");
    s_disc_bytes_reported = bytes;
}

static void report(StallEvent event, uint64_t stuck_ms)
{
    const char* stage = (const char*)s_stage;
    unsigned long frame = (unsigned long)s_frame;
    switch (event) {
        case STALL_BEGAN:
            fprintf(stderr, "[watchdog] the game thread has been stuck for %llu s in frame %lu at: %s\n",
                    (unsigned long long)(stuck_ms / 1000u), frame, stage);
            report_disc();
            break;
        case STALL_GOES_ON:
            fprintf(stderr, "[watchdog] still stuck after %llu s (frame %lu) at: %s\n",
                    (unsigned long long)(stuck_ms / 1000u), frame, stage);
            report_disc();
            break;
        case STALL_ENDED:
            fprintf(stderr, "[watchdog] the game thread moved on (frame %lu, now at: %s)\n", frame, stage);
            break;
        default:
            break;
    }
}

static void watch(void* arg)
{
    (void)arg;
    StallWatch w = stall_watch_start(s_seq, clock3ds_ms());
    while (!s_stop) {
        svcSleepThread(WATCH_PERIOD_NS);
        uint64_t since = w.since_ms;
        w = stall_watch_check(w, s_seq, clock3ds_ms(), STALL_MS, REPEAT_MS);
        report(w.event, w.event == STALL_ENDED ? 0 : clock3ds_ms() - since);
    }
}

void trace3ds_start(bool detailed)
{
    s_detailed = detailed;
    if (!detailed) {
        return;
    }
    s_thread = threadCreate(watch, NULL, WATCH_STACK, WATCH_PRIO, WATCH_CORE, false);
    fprintf(stderr, "[trace] watchdog %s\n", s_thread != NULL ? "on core 2" : "not started (no core 2)");
}

void trace3ds_stop(void)
{
    if (s_thread == NULL) {
        return;
    }
    s_stop = true;
    threadJoin(s_thread, U64_MAX);
    threadFree(s_thread);
    s_thread = NULL;
}
