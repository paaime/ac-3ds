#ifndef AC3DS_DISC_CHECK_H
#define AC3DS_DISC_CHECK_H

#include <stdint.h>

#include <stdbool.h>

#define DISC_ID_TEXT      7     /* the 6-character game ID and its end */
#define DISC_HEADER_BYTES 0x430 /* up to the file table's offset and size */

typedef enum {
    DISC_ID_OK,
    DISC_ID_OTHER_REGION,
    DISC_ID_OTHER_REVISION,
    DISC_ID_OTHER_GAME,
} DiscIdResult;

/* Extensions in any case; hidden files (macOS's "._" copies) are refused. */
bool disc_check_name(const char* name);

DiscIdResult disc_check_id(const unsigned char header[8]);

/* From the ID's fourth letter; NULL for an unknown region. */
const char* disc_check_region(char region);

bool disc_check_complete(const unsigned char header[DISC_HEADER_BYTES], uint64_t file_size);

void disc_check_id_text(const unsigned char header[8], char out[DISC_ID_TEXT]);

#endif /* AC3DS_DISC_CHECK_H */
