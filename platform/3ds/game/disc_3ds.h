#ifndef AC3DS_DISC_3DS_H
#define AC3DS_DISC_3DS_H

#include <dirent.h>
#include <stdbool.h>

/* False after telling the player why. */
bool disc3ds_open(void);

const char* disc3ds_name(void);

/* pc_disc.c's readdir (renamed in CMakeLists.txt). */
struct dirent* ac3ds_disc_readdir(DIR* dir);

#endif /* AC3DS_DISC_3DS_H */
