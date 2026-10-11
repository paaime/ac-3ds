/* settings_3ds.c - the port's settings in settings.ini (see settings_3ds.h). */
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

#define SETTINGS_FILE  "settings.ini" /* in the cwd, the data folder */
#define SAVE_DELAY_MS  2000u          /* after the last change: a burst of taps is one write */

int ac3ds_fclose_durable(FILE* f); /* save_3ds.c */

static Settings s_settings;
static bool s_from_file;
static Debounce s_unsaved; /* changed since settings.ini was written */
int g_ac3ds_fast_text;      /* pc_platform.h: read by the game's message code */
static aptHookCookie s_apt_hook;

/* Settings the player build doesn't offer count as off, whatever the file says. */
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

/* What applies while the game runs (the rest is read where it's used). */
static void apply(Settings s)
{
    g_pc_settings.disable_resetti = effective(s, SETTING_SKIP_RESETTI); /* pc_m_card.c, when a town loads */
    g_ac3ds_fast_text = effective(s, SETTING_FAST_TEXT);                /* m_msg_cursol.c_inc, each frame */
    gxr_set_sharp_textures(effective(s, SETTING_SHARP_TEXTURES));
    GxrPictureShape shape = picture_shape(s);
    gxr_set_picture_shape(shape);
    /* The game culls at the edge of what it shows: m_actor.c and emu64.c take it
     * from the PC port's window size, here the top screen's shape in Wide. */
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

/* A change still waiting is written now, as the game may not come back (APT
 * hooks run on the game thread, between frames). */
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
        s_unsaved = debounce_change(s_unsaved, clock3ds_ms()); /* try again after the delay */
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
