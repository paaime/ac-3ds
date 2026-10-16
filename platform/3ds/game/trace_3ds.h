#ifndef AC3DS_TRACE_3DS_H
#define AC3DS_TRACE_3DS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* stage: a string literal naming the step about to run. */
void trace3ds_mark(const char* stage);
void trace3ds_frame(uint32_t frame);

typedef struct {
    uint32_t draws, vertices, texture_uploads, texture_failures, efb_copies, efb_copy_draws, fog_tables, cmd_words,
        dropped_draws, max_draw_cmd_words;
    bool bottom_drawn;
} TraceFrameStats;
void trace3ds_frame_stats(TraceFrameStats s);

/* pc_disc.c's fread, renamed in CMakeLists.txt. */
size_t ac3ds_disc_fread(void* ptr, size_t size, size_t count, FILE* file);

void trace3ds_start(bool detailed);
void trace3ds_stop(void);

#endif /* AC3DS_TRACE_3DS_H */
