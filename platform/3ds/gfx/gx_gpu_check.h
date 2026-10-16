#ifndef AC3DS_GX_GPU_CHECK_H
#define AC3DS_GX_GPU_CHECK_H

#include <stdbool.h>
#include <stdint.h>

void gxg_arm(uint32_t frames);
bool gxg_active(void);
/* false if the GPU hasn't finished within 2 s. */
bool gxg_run_and_wait(void);
void gxg_empty_queue(void);
void gxg_draw_failed(uint32_t draw);
/* Call after C3D_FrameEnd. */
bool gxg_frame_end(uint32_t draws);

/* Call with the GPU idle. */
void gxg_log_gpu_state(const char* when);

#endif /* AC3DS_GX_GPU_CHECK_H */
