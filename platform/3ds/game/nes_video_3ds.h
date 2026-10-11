#ifndef AC3DS_NES_VIDEO_3DS_H
#define AC3DS_NES_VIDEO_3DS_H

/* The NES picture on the top screen (nes_video_3ds.c):
 * the 224 lines the PC port shows, drawn 1:1 in the middle. */

#include <stdint.h>

/* `frame`: 256 x 240 RGB565 (red in the high bits, the PICA's GPU_RGB565). */
void nes_video_show(const uint16_t* frame);
void nes_video_release(void);

#endif /* AC3DS_NES_VIDEO_3DS_H */
