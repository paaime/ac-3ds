#ifndef AC3DS_LOOK_H
#define AC3DS_LOOK_H

#include <stdint.h>

#define LOOK_FACES 8
#define LOOK_TONES 9

uint8_t look_floor_to_save(int tone);
int look_floor_from_save(uint8_t b);

int look_face_step(int face, int delta);
int look_tone_step(int tone, int delta);

#endif /* AC3DS_LOOK_H */
