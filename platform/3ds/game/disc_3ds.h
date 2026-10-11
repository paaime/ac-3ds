#ifndef AC3DS_DISC_3DS_H
#define AC3DS_DISC_3DS_H

/* The player's disc image, checked before the game starts (disc_3ds.c). */

#include <dirent.h>
#include <stdbool.h>

/* Checks it is Animal Crossing USA's first release; false after showing why not. */
bool disc3ds_open(void);

const char* disc3ds_name(void);

/* pc_disc.c's readdir (renamed in CMakeLists.txt): skips hidden and "._" files. */
struct dirent* ac3ds_disc_readdir(DIR* dir);

#endif /* AC3DS_DISC_3DS_H */
