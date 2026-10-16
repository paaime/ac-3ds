#ifndef AC3DS_SETTINGS_3DS_H
#define AC3DS_SETTINGS_3DS_H

#include <stdbool.h>

#include "settings_file.h"

/* Once the data folder is the cwd, before the log opens (verbose_log decides how it opens). */
void settings3ds_load(void);
void settings3ds_log(void);

Settings settings3ds_get(void);
bool settings3ds_on(SettingId id);

void settings3ds_toggle(SettingId id);
bool settings3ds_save(void);
void settings3ds_frame(void);

#endif /* AC3DS_SETTINGS_3DS_H */
