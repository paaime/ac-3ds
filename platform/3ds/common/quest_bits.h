#ifndef AC3DS_QUEST_BITS_H
#define AC3DS_QUEST_BITS_H

/* Repacks the bitfield half (bytes 0-1 only) of a GCI quest header (mQst_base_c): its
 * six bitfields run from the top bit on GameCube, from the bottom bit on ARM GCC. */

#include <stdbool.h>
#include <stdint.h>

void quest_bits_repack(uint8_t* bytes, bool from_gc);

#endif /* AC3DS_QUEST_BITS_H */
