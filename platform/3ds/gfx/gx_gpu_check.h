#ifndef AC3DS_GX_GPU_CHECK_H
#define AC3DS_GX_GPU_CHECK_H

/* The renderer's draw-by-draw GPU check (gxr_gpu_check):
 * in a checked frame every draw goes to the GPU at once and must finish within 2 s. */

#include <stdbool.h>
#include <stdint.h>

void gxg_arm(uint32_t frames);
bool gxg_active(void);
/* Runs citro3d's queue and waits for the GPU: false after 2 s. */
bool gxg_run_and_wait(void);
/* As C3D_FrameBegin does: the queue's 32 entries would otherwise run out. */
void gxg_empty_queue(void);
/* Logged, and the check stops. */
void gxg_draw_failed(uint32_t draw);
/* After C3D_FrameEnd: the frame's end (screen transfers) must finish too. */
bool gxg_frame_end(uint32_t draws);

/* Logs PICA registers citro3d doesn't set at C3D_Init. With the GPU idle. */
void gxg_log_gpu_state(const char* when);

#endif /* AC3DS_GX_GPU_CHECK_H */
