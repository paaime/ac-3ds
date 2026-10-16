#include "pc_platform.h"
#include "pc_settings.h"

#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "debounce.h"
#include "gx_render.h"
#include "settings_3ds.h"
#include "texdump_3ds.h"
#include "clock_3ds.h"

#define SETTINGS_FILE  "settings.ini"
#define SAVE_DELAY_MS  2000u /* after the last change: a burst of taps is one write */

int ac3ds_fclose_durable(FILE* f);

static Settings s_settings;
static bool s_from_file;
static Debounce s_unsaved;
int g_ac3ds_fast_text;
static aptHookCookie s_apt_hook;

static bool effective(Settings s, SettingId id)
{
    return settings_get(s, id) && settings_offered(id, AC3DS_DEBUG);
}

static GxrPictureShape picture_shape(Settings s)
{
    switch (settings_value(s, SETTING_PICTURE_SHAPE)) {
    case PICTURE_4_3:
        return GXR_PICTURE_4_3;
    case PICTURE_STRETCH:
        return GXR_PICTURE_STRETCH;
    default:
        return GXR_PICTURE_WIDE;
    }
}

static void apply(Settings s)
{
    g_pc_settings.disable_resetti = effective(s, SETTING_SKIP_RESETTI);
    g_ac3ds_fast_text = effective(s, SETTING_FAST_TEXT);
    gxr_set_sharp_textures(effective(s, SETTING_SHARP_TEXTURES));
    GxrPictureShape shape = picture_shape(s);
    gxr_set_picture_shape(shape);
    /* m_actor.c and emu64.c cull at the PC window's edge: in Wide, the top screen's shape. */
    g_pc_window_w = shape == GXR_PICTURE_WIDE ? PC_SCREEN_HEIGHT * GSP_SCREEN_HEIGHT_TOP / GSP_SCREEN_WIDTH
                                              : PC_SCREEN_WIDTH;
    g_pc_window_h = PC_SCREEN_HEIGHT;
    texdump3ds_enable(effective(s, SETTING_DUMP_TEXTURES));
}

static bool read_file(char* text, size_t size)
{
    FILE* f = fopen(SETTINGS_FILE, "r");
    if (f == NULL) {
        return false;
    }
    size_t n = fread(text, 1, size - 1, f);
    text[n] = '\0';
    fclose(f);
    return true;
}

static bool write_file(Settings s)
{
    char text[SETTINGS_TEXT_MAX];
    size_t n = settings_format(s, text, sizeof(text));
    FILE* f = n != 0 ? fopen(SETTINGS_FILE, "w") : NULL;
    if (f == NULL) {
        return false;
    }
    bool written = fwrite(text, 1, n, f) == n;
    return ac3ds_fclose_durable(f) == 0 && written;
}

/* The game may not come back: APT hooks run on the game thread, between frames. */
static void on_apt(APT_HookType type, void* param)
{
    (void)param;
    if (type == APTHOOK_ONSUSPEND || type == APTHOOK_ONSLEEP || type == APTHOOK_ONEXIT) {
        settings3ds_save();
    }
}

void settings3ds_load(void)
{
    char text[SETTINGS_TEXT_MAX];
    s_from_file = read_file(text, sizeof(text));
    s_settings = s_from_file ? settings_parse(settings_defaults(), text) : settings_defaults();
    apply(s_settings);
    aptHook(&s_apt_hook, on_apt, NULL);
}

void settings3ds_log(void)
{
    fprintf(stderr, "[settings] from %s:", s_from_file ? SETTINGS_FILE : "the defaults (no " SETTINGS_FILE " yet)");
    for (int id = 0; id < SETTING_COUNT; id++) {
        fprintf(stderr, " %s %s;", settings_label((SettingId)id), settings_value_text(s_settings, (SettingId)id));
    }
    fprintf(stderr, "\n");
}

Settings settings3ds_get(void)
{
    return s_settings;
}

bool settings3ds_on(SettingId id)
{
    return effective(s_settings, id);
}

void settings3ds_toggle(SettingId id)
{
    s_settings = settings_toggle(s_settings, id);
    s_unsaved = debounce_change(s_unsaved, clock3ds_ms());
    apply(s_settings);
    fprintf(stderr, "[settings] %s: %s\n", settings_label(id), settings_value_text(s_settings, id));
}

bool settings3ds_save(void)
{
    if (!s_unsaved.pending) {
        return true;
    }
    bool written = write_file(s_settings);
    if (written) {
        s_unsaved = debounce_done(s_unsaved);
    } else {
        s_unsaved = debounce_change(s_unsaved, clock3ds_ms());
    }
    fprintf(stderr, "[settings] %s\n", written ? "saved to " SETTINGS_FILE : SETTINGS_FILE " couldn't be written");
    return written;
}

void settings3ds_frame(void)
{
    if (debounce_due(s_unsaved, clock3ds_ms(), SAVE_DELAY_MS)) {
        settings3ds_save();
    }
}
