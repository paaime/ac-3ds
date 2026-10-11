/* pc_profiler.h — 3DS version of pc/include/pc_profiler.h, with SDL's Uint64
 * defined here; graph.c's timers feed game/profiler_3ds.c. */
#ifndef PC_PROFILER_H
#define PC_PROFILER_H

#include "pc_platform.h"

typedef u64 Uint64;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PC_PROF_TIMER_GX_BEGIN,
    PC_PROF_TIMER_DISPLAY_LIST,
    PC_PROF_TIMER_GX_FLUSH,
    PC_PROF_TIMER_BUFFER_UPLOAD,
    PC_PROF_TIMER_UNIFORM_UPLOAD,
    PC_PROF_TIMER_UNIFORM_LOOKUP,
    PC_PROF_TIMER_TEXTURE_BIND,
    PC_PROF_TIMER_SHADER_SWITCH,
    PC_PROF_TIMER_GL_STATE,
    PC_PROF_TIMER_DRAW_SUBMIT,
    PC_PROF_TIMER_POLL_EVENTS,
    PC_PROF_TIMER_SWAP,
    PC_PROF_TIMER_PACE,
    PC_PROF_TIMER_GAME_LOGIC,
    PC_PROF_TIMER_EMU64,
    PC_PROF_TIMER_TEXOBJ,
    PC_PROF_TIMER_DRAW_FINISH,
    PC_PROF_TIMER_AUDIO_FRAME,
    PC_PROF_TIMER_JW_FRAME,
    PC_PROF_TIMER_EFB_COPY,
    PC_PROF_TIMER_COUNT
} PCProfilerTimer;

extern int g_pc_profile_enabled;
extern int g_pc_profile_interval;

void pc_profiler_begin_frame(void);
void pc_profiler_end_frame(double frame_ms, int audio_fill);

void pc_profiler_add_time_slow(PCProfilerTimer timer, Uint64 start);
void pc_profiler_add_count_draw_slow(int vertices, int indices);
void pc_profiler_add_count_flush_slow(void);
void pc_profiler_add_count_shader_switch_slow(void);
void pc_profiler_add_count_uniform_slow(void);
void pc_profiler_add_count_uniform_lookup_slow(void);
void pc_profiler_add_count_texture_bind_slow(void);
void pc_profiler_add_count_buffer_upload_slow(size_t bytes);
void pc_profiler_add_count_state_change_slow(void);
void pc_profiler_add_dirty_mask_slow(unsigned int dirty);

uint64_t prof3ds_now(void); /* the system tick */

/* Hot-path wrappers: with profiling off these cost one predicted branch, no call. */
static inline Uint64 pc_profiler_begin_timer(void) {
    return g_pc_profile_enabled ? prof3ds_now() : 0;
}
static inline void pc_profiler_add_time(PCProfilerTimer timer, Uint64 start) {
    if (g_pc_profile_enabled) pc_profiler_add_time_slow(timer, start);
}
static inline void pc_profiler_add_count_draw(int vertices, int indices) {
    if (g_pc_profile_enabled) pc_profiler_add_count_draw_slow(vertices, indices);
}
static inline void pc_profiler_add_count_flush(void) {
    if (g_pc_profile_enabled) pc_profiler_add_count_flush_slow();
}
static inline void pc_profiler_add_count_shader_switch(void) {
    if (g_pc_profile_enabled) pc_profiler_add_count_shader_switch_slow();
}
static inline void pc_profiler_add_count_uniform(void) {
    if (g_pc_profile_enabled) pc_profiler_add_count_uniform_slow();
}
static inline void pc_profiler_add_count_uniform_lookup(void) {
    if (g_pc_profile_enabled) pc_profiler_add_count_uniform_lookup_slow();
}
static inline void pc_profiler_add_count_texture_bind(void) {
    if (g_pc_profile_enabled) pc_profiler_add_count_texture_bind_slow();
}
static inline void pc_profiler_add_count_buffer_upload(size_t bytes) {
    if (g_pc_profile_enabled) pc_profiler_add_count_buffer_upload_slow(bytes);
}
static inline void pc_profiler_add_count_state_change(void) {
    if (g_pc_profile_enabled) pc_profiler_add_count_state_change_slow();
}
static inline void pc_profiler_add_dirty_mask(unsigned int dirty) {
    if (g_pc_profile_enabled) pc_profiler_add_dirty_mask_slow(dirty);
}

#ifdef __cplusplus
}
#endif

#endif /* PC_PROFILER_H */
