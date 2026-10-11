#ifndef AC3DS_TRACE_3DS_H
#define AC3DS_TRACE_3DS_H

/* Where the game thread is (trace_3ds.c), for hangs on hardware. With the
 * "Detailed log" setting a watchdog logs where it's stuck; else a mark is a store. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* `stage`: a string literal naming the step about to run. */
void trace3ds_mark(const char* stage);
void trace3ds_frame(uint32_t frame);

/* Logged for the first frames: which frame's contents a GPU hang choked on. */
typedef struct {
    uint32_t draws, vertices, texture_uploads, texture_failures, efb_copies, efb_copy_draws, fog_tables, cmd_words,
        dropped_draws, max_draw_cmd_words;
    bool bottom_drawn;
} TraceFrameStats;
void trace3ds_frame_stats(TraceFrameStats s);

/* pc_disc.c's fread (renamed in CMakeLists.txt), counted to tell a slow load from a hang. */
size_t ac3ds_disc_fread(void* ptr, size_t size, size_t count, FILE* file);

/* After the log is open; `detailed` starts the watchdog. */
void trace3ds_start(bool detailed);
void trace3ds_stop(void);

#endif /* AC3DS_TRACE_3DS_H */
