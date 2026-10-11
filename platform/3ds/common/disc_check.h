#ifndef AC3DS_DISC_CHECK_H
#define AC3DS_DISC_CHECK_H

/* The player's disc image (for game/disc_3ds.c): file names, the header's ID
 * (the port runs Animal Crossing USA's first release only: GAFE01, revision 0)
 * and whether an .iso/.gcm is whole. */

#include <stdint.h>

#include <stdbool.h>

#define DISC_ID_TEXT      7     /* the 6-character game ID and its end */
#define DISC_HEADER_BYTES 0x430 /* up to the file table's offset and size */

typedef enum {
    DISC_ID_OK,
    DISC_ID_OTHER_REGION,   /* Animal Crossing (or Doubutsu no Mori e+), not the USA version */
    DISC_ID_OTHER_REVISION,
    DISC_ID_OTHER_GAME,
} DiscIdResult;

/* .iso, .gcm or .ciso (any case), and not a hidden file (macOS's "._" copies). */
bool disc_check_name(const char* name);

/* The header's first 8 bytes: the game ID, the disc number, the revision. */
DiscIdResult disc_check_id(const unsigned char header[8]);

/* From the ID's fourth letter; NULL for an unknown region. */
const char* disc_check_region(char region);

/* False for a copy cut short before the end of the disc's file table. */
bool disc_check_complete(const unsigned char header[DISC_HEADER_BYTES], uint64_t file_size);

void disc_check_id_text(const unsigned char header[8], char out[DISC_ID_TEXT]);

#endif /* AC3DS_DISC_CHECK_H */
