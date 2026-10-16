#ifndef AC3DS_SETTINGS_FILE_H
#define AC3DS_SETTINGS_FILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SETTINGS_TEXT_MAX 512 /* settings.ini, comment included */

typedef enum {
    SETTING_SKIP_RESETTI,
    SETTING_PICTURE_SHAPE,
    SETTING_CONTINUOUS_CAMERA,
    SETTING_SHARP_TEXTURES,
    SETTING_FAST_TEXT,
    SETTING_SHOP_WITHOUT_VISITOR,
    SETTING_NO_WEEDS,
    SETTING_BOTTOM_LIGHT_SAVER,
    SETTING_FAST_FLUSH, /* flush only what the CPU wrote for the GPU */
    SETTING_ONE_CORE_AUDIO,
    SETTING_VERBOSE_LOG,
    SETTING_DUMP_TEXTURES,
    SETTING_COUNT
} SettingId;

/* SETTING_PICTURE_SHAPE's choices, in their tap order. */
typedef enum {
    PICTURE_WIDE,
    PICTURE_4_3,
    PICTURE_STRETCH,
    PICTURE_SHAPE_COUNT
} PictureShape;

typedef struct {
    uint8_t value[SETTING_COUNT]; /* a switch: 0 off, 1 on; a choice: its index */
} Settings;

typedef enum {
    SETTINGS_PAGE_GAME,
    SETTINGS_PAGE_TESTING,
    SETTINGS_PAGE_COUNT
} SettingsPage;

#define SETTINGS_PAGE_MAX 8

Settings settings_defaults(void);
/* A switch is on; a choice is past its first. */
bool settings_get(Settings s, SettingId id);
int settings_value(Settings s, SettingId id);
int settings_choice_count(SettingId id);
Settings settings_set(Settings s, SettingId id, bool on);
Settings settings_set_value(Settings s, SettingId id, int value);
Settings settings_toggle(Settings s, SettingId id);

Settings settings_parse(Settings base, const char* text);
/* NUL-terminated; returns the length, or 0 if it doesn't fit. */
size_t settings_format(Settings s, char* out, size_t size);

const char* settings_label(SettingId id);
const char* settings_value_text(Settings s, SettingId id);
const char* settings_note(SettingId id);
bool settings_is_switch(SettingId id);

const char* settings_page_name(SettingsPage page);
int settings_page_size(SettingsPage page);
SettingsPage settings_page_of(SettingId id);
int settings_page_count(bool developer);
/* A setting a build doesn't offer counts as off; the file keeps its value. */
bool settings_offered(SettingId id, bool developer);
SettingId settings_on_page(SettingsPage page, int n);

#endif /* AC3DS_SETTINGS_FILE_H */
