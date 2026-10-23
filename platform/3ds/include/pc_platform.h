/* Replaces pc/include/pc_platform.h: force-included under the same guard, so the PC one is never used. */
#ifndef PC_PLATFORM_H
#define PC_PLATFORM_H

/* 32-bit required: decomp code (JSystem, emu64) casts pointers to u32 */
#include <stdint.h>
#if UINTPTR_MAX != 0xFFFFFFFFu
#error "This project must be compiled as 32-bit (pointer size != 4 bytes)"
#endif

/* First: libultra/osContPad.h #undefs errno for a struct field, so its re-include of <errno.h> must be a no-op. */
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pc_types.h"

#ifndef TARGET_3DS
#define TARGET_3DS 1
#endif

/* libctru's osGetTime (ms since 1900, used by its time()) clashes with libultra's at link time (os_3ds.c). */
#define osGetTime ac_osGetTime

#define PC_GC_WIDTH      640
#define PC_GC_HEIGHT     480
#define PC_SCREEN_WIDTH  PC_GC_WIDTH
#define PC_SCREEN_HEIGHT PC_GC_HEIGHT
#define PC_WINDOW_TITLE  "Animal Crossing"

#define PC_MAIN_MEMORY_SIZE (24 * 1024 * 1024)
#define PC_ARAM_SIZE        (16 * 1024 * 1024)
#define PC_FIFO_SIZE        (256 * 1024)

#define PC_SPEEDHACK_MULTIPLIER 10.0

#define PC_PI          3.14159265358979323846
#define PC_PIf         3.14159265358979323846f
#define PC_DEG_TO_RAD  (PC_PI / 180.0)
#define PC_DEG_TO_RADf (PC_PIf / 180.0f)

#define GC_BUS_CLOCK   162000000u
#define GC_CORE_CLOCK  486000000u
#define GC_TIMER_CLOCK (GC_BUS_CLOCK / 4)

#ifdef __cplusplus
extern "C" {
#endif

extern int g_pc_running;
extern int g_pc_verbose;
extern int g_pc_frame_limit_override;
extern int g_pc_speedhack_enabled;
extern int g_pc_time_override;
extern int g_pc_min_override;
extern int g_pc_sec_override;
extern int g_pc_date_month;
extern int g_pc_date_day;
extern int g_pc_date_year;
extern int g_pc_weather_override;
extern int g_pc_weather_intensity_override;
extern u32 g_frame_limiter;

extern int g_ac3ds_fast_text;

/* Set during the boat ride, which switches borderless acres itself. */
extern int g_ac3ds_boat_ride;

/* The Save button's return spot for the save room's "continue"; FALSE: the door. */
extern int ac3ds_save_return(short* x, short* y, short* z, unsigned char* direction);
extern void ac3ds_save_return_clear(void);

/* The town's ordinance, as the game's code asks for it (game/ordinance_3ds.c). */
extern int ac3ds_ordinance_villager_sec(int now_sec);
extern int ac3ds_ordinance_shop_open_hour(int hour);
extern int ac3ds_ordinance_shop_close_hour(int hour);
extern int ac3ds_ordinance_tailor_open(int now_sec);
extern int ac3ds_ordinance_weeds_grow(void);

/* The port's messages follow the game's 0x3F91 (m_msg_data.h's MSG_MAX, raised to AC3DS_MSG_END). */
#define AC3DS_MSG_FIRST 0x3F91
#define AC3DS_MSG_END   (AC3DS_MSG_FIRST + 0x40)
extern int ac3ds_msg_build(int index, unsigned char* buf, int size);
extern void ac3ds_msg_patch(int index, unsigned char* buf, int len, int size);

/* Pelly's counter menu once ac3ds_msg_patch gave it Ordinances (game/post_office_3ds.c). */
#define AC3DS_POST_CHOICE_ORDINANCES 6 /* past mChoice_CHOICE5 */
extern int ac3ds_post_menu_choice(int choice);
/* TRUE once the ordinance talk is over: Pelly's "anything else?" comes next. */
extern int ac3ds_post_ordinance_talk(void);

/* The 640x480 EFB the game projects for. */
extern int g_pc_window_w;
extern int g_pc_window_h;
void pc_platform_update_window_size(void);

/* As in pc/include/pc_platform.h; g_pc_widescreen_stretch 0 is hor+. */
#define PC_NOOP_WIDESCREEN_STRETCH     0xAC5701u
#define PC_NOOP_WIDESCREEN_STRETCH_OFF 0xAC5700u
extern int g_pc_widescreen_stretch;

void pc_platform_init(void);
void pc_platform_shutdown(void);
void pc_platform_swap_buffers(void);
int pc_platform_poll_events(void);

/* Executable image (0x00100000 to .bss end), for seg2k0 pointer disambiguation. */
extern unsigned int pc_image_base;
extern unsigned int pc_image_end;

extern int g_pc_model_viewer;
extern int g_pc_model_viewer_start;
extern int g_pc_model_viewer_no_cull;

extern int pc_emu64_frame_cmds;
extern int pc_emu64_frame_noop_cmds;
extern int pc_emu64_frame_tri_cmds;
extern int pc_emu64_frame_vtx_cmds;
extern int pc_emu64_frame_dl_cmds;
extern int pc_emu64_frame_cull_visible;
extern int pc_emu64_frame_cull_rejected;
extern int pc_gx_draw_call_count;

extern int pc_save_loaded;
int pc_audio_get_buffer_fill(void);
int pc_audio_is_active(void);
void pc_audio_set_paused(int paused);
void pc_audio_shutdown(void);
void pc_audio_start_producer_thread(void);
void pc_audio_mq_init(void);
void pc_audio_mq_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_PLATFORM_H */
