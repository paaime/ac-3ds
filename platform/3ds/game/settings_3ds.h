#ifndef AC3DS_SETTINGS_3DS_H
#define AC3DS_SETTINGS_3DS_H

/* The port's settings (common/settings_file.h) in settings.ini: written 2 s
 * after the last change, when the page is left, before HOME or sleep, at exit. */

#include <stdbool.h>

#include "settings_file.h"

/* Once the data folder is the cwd, before the log opens (verbose_log decides
 * how it opens): quiet. */
void settings3ds_load(void);
/* Once the log is open. */
void settings3ds_log(void);

Settings settings3ds_get(void);
bool settings3ds_on(SettingId id);

/* Flips one setting and applies what applies now; settings3ds_save writes it. */
void settings3ds_toggle(SettingId id);
/* Writes settings.ini if a setting changed; false if it couldn't be. */
bool settings3ds_save(void);
/* Once a frame: writes a change once it has waited long enough. */
void settings3ds_frame(void);

#endif /* AC3DS_SETTINGS_3DS_H */
