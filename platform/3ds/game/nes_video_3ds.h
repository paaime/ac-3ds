#ifndef AC3DS_NES_VIDEO_3DS_H
#define AC3DS_NES_VIDEO_3DS_H

#include <stdint.h>

/* frame: 256x240 RGB565, red in the high bits (the PICA's GPU_RGB565). */
void nes_video_show(const uint16_t* frame);
void nes_video_release(void);

#endif /* AC3DS_NES_VIDEO_3DS_H */
