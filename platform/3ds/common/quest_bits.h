#ifndef AC3DS_QUEST_BITS_H
#define AC3DS_QUEST_BITS_H

/* mQst_base_c's six bitfields (bytes 0-1) run from the top bit on GameCube, from the bottom on ARM GCC. */

#include <stdbool.h>
#include <stdint.h>

void quest_bits_repack(uint8_t* bytes, bool from_gc);

#endif /* AC3DS_QUEST_BITS_H */
