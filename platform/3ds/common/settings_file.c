#include "settings_file.h"

#include <stdio.h>
#include <string.h>

#include "light_timer.h"

#define LINE_MAX_CHARS 80
#define CHOICES_MAX    3 /* Picture shape's */
#define FILE_HEADER      "# Animal Crossing 3DS settings (also changed from the bottom screen's Options page)\n"
#define SWITCH_OFF_TEXT  "Off"

#define NOTE_TOWN_LOAD "Applies the next time your town loads"
#define NOTE_RESTART   "Applies the next time the game starts"
#define NOTE_NOW       "Applies now"
#define NOTE_DETAILED  "More log and GPU checks, from the next start"
#define STRINGIFY_(x)  #x
#define STRINGIFY(x)   STRINGIFY_(x)
#define NOTE_LIGHT     "Goes dark after " STRINGIFY(LIGHT_SAVER_TIMEOUT_S) " s, a touch lights it"

typedef struct {
    const char* key;
    const char* label;
    const char* choices[CHOICES_MAX]; /* NULL after the last; lower case in the file */
    const char* note;
    SettingsPage page;
    uint8_t default_value;
} SettingInfo;

#define SWITCH { SWITCH_OFF_TEXT, "On" }

static const SettingInfo k_settings[SETTING_COUNT] = {
    [SETTING_SKIP_RESETTI] = { "skip_resetti", "Skip Mr. Resetti", SWITCH, NOTE_TOWN_LOAD, SETTINGS_PAGE_GAME, 0 },
    [SETTING_PICTURE_SHAPE] = { "picture_shape", "Picture shape", { "Wide", "4:3", "Stretch" },
                                "Wide sees more, Stretch fills, 4:3 has bars", SETTINGS_PAGE_GAME, PICTURE_WIDE },
    [SETTING_CONTINUOUS_CAMERA] = { "continuous_camera", "Continuous camera", SWITCH,
                                    "Off: it stops at an acre's edge, then slides", SETTINGS_PAGE_GAME, 1 },
    [SETTING_SHARP_TEXTURES] = { "sharp_textures", "Sharp textures", SWITCH, "Crisp, unsmoothed textures in 3D",
                                 SETTINGS_PAGE_GAME, 0 },
    [SETTING_FAST_TEXT] = { "fast_text", "Faster text", SWITCH, "Dialogue at the fast speed, as with B",
                            SETTINGS_PAGE_GAME, 0 },
    [SETTING_SHOP_WITHOUT_VISITOR] = { "shop_without_visitor", "Nookington's without a visitor", SWITCH,
                                       "Next renovation: no visitor needed", SETTINGS_PAGE_GAME, 0 },
    [SETTING_NO_WEEDS] = { "no_weeds", "No weeds", SWITCH, "Pulled each time you enter your town", SETTINGS_PAGE_GAME,
                           0 },
    [SETTING_BOTTOM_LIGHT_SAVER] = { "bottom_light_saver", "Bottom screen light saver", SWITCH, NOTE_LIGHT,
                                     SETTINGS_PAGE_GAME, 0 },
    [SETTING_FAST_FLUSH] = { "fast_flush", "Fast GPU flush", SWITCH, NOTE_NOW, SETTINGS_PAGE_TESTING, 0 },
    [SETTING_ONE_CORE_AUDIO] = { "one_core_audio", "Sound on one core", SWITCH, NOTE_RESTART, SETTINGS_PAGE_TESTING,
                                 0 },
    [SETTING_VERBOSE_LOG] = { "verbose_log", "Detailed log", SWITCH, NOTE_DETAILED, SETTINGS_PAGE_TESTING, 0 },
    [SETTING_DUMP_TEXTURES] = { "dump_textures", "Dump textures", SWITCH, "Saves each texture shown to the dump folder",
                                SETTINGS_PAGE_TESTING, 0 },
};

static const char* const k_page_names[SETTINGS_PAGE_COUNT] = {
    [SETTINGS_PAGE_GAME] = "Game",
    [SETTINGS_PAGE_TESTING] = "Testing",
};

static bool valid(SettingId id)
{
    return (unsigned)id < (unsigned)SETTING_COUNT; /* also rejects negative ids */
}

Settings settings_defaults(void)
{
    Settings s;
    memset(&s, 0, sizeof(s));
    for (int id = 0; id < SETTING_COUNT; id++) {
        s.value[id] = k_settings[id].default_value;
    }
    return s;
}

int settings_choice_count(SettingId id)
{
    int n = 0;
    while (valid(id) && n < CHOICES_MAX && k_settings[id].choices[n] != NULL) {
        n++;
    }
    return n;
}

bool settings_get(Settings s, SettingId id)
{
    return valid(id) && s.value[id] != 0;
}

int settings_value(Settings s, SettingId id)
{
    return valid(id) ? s.value[id] : 0;
}

Settings settings_set_value(Settings s, SettingId id, int value)
{
    Settings out = s;
    if (value >= 0 && value < settings_choice_count(id)) {
        out.value[id] = (uint8_t)value;
    }
    return out;
}

Settings settings_set(Settings s, SettingId id, bool on)
{
    return settings_set_value(s, id, on ? 1 : 0);
}

Settings settings_toggle(Settings s, SettingId id)
{
    int count = settings_choice_count(id);
    return count == 0 ? s : settings_set_value(s, id, (settings_value(s, id) + 1) % count);
}

static char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static bool same_word(const char* a, size_t a_len, const char* b)
{
    if (strlen(b) != a_len) {
        return false;
    }
    for (size_t i = 0; i < a_len; i++) {
        if (lower(a[i]) != lower(b[i])) {
            return false;
        }
    }
    return true;
}

static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

static void trim(const char** start, const char** end)
{
    while (*start < *end && is_space(**start)) {
        (*start)++;
    }
    while (*end > *start && is_space((*end)[-1])) {
        (*end)--;
    }
}

static Settings apply_line(Settings s, const char* line, const char* end)
{
    trim(&line, &end);
    const char* equals = memchr(line, '=', (size_t)(end - line));
    if (line == end || *line == '#' || equals == NULL) {
        return s;
    }
    const char* key_end = equals;
    const char* value = equals + 1;
    trim(&line, &key_end);
    trim(&value, &end);
    for (int id = 0; id < SETTING_COUNT; id++) {
        const SettingInfo* info = &k_settings[id];
        if (!same_word(line, (size_t)(key_end - line), info->key)) {
            continue;
        }
        size_t len = (size_t)(end - value);
        int count = settings_choice_count((SettingId)id);
        for (int v = 0; v < count; v++) {
            if (same_word(value, len, info->choices[v])) {
                return settings_set_value(s, (SettingId)id, v);
            }
        }
        if (len == 1 && *value >= '0' && *value < '0' + count) {
            return settings_set_value(s, (SettingId)id, *value - '0');
        }
        return s;
    }
    return s;
}

Settings settings_parse(Settings base, const char* text)
{
    Settings s = base;
    if (text == NULL) {
        return s;
    }
    while (*text != '\0') {
        const char* end = strchr(text, '\n');
        end = end != NULL ? end : text + strlen(text);
        if (end - text <= LINE_MAX_CHARS) {
            s = apply_line(s, text, end);
        }
        text = *end == '\n' ? end + 1 : end;
    }
    return s;
}

static void lower_into(char* out, size_t size, const char* word)
{
    size_t i = 0;
    for (; word[i] != '\0' && i + 1 < size; i++) {
        out[i] = lower(word[i]);
    }
    out[i] = '\0';
}

size_t settings_format(Settings s, char* out, size_t size)
{
    if (out == NULL || size == 0) {
        return 0;
    }
    int n = snprintf(out, size, "%s", FILE_HEADER);
    if (n < 0 || (size_t)n >= size) {
        return 0;
    }
    size_t used = (size_t)n;
    for (int id = 0; id < SETTING_COUNT; id++) {
        const SettingInfo* info = &k_settings[id];
        char value[LINE_MAX_CHARS];
        lower_into(value, sizeof(value), info->choices[s.value[id]]);
        n = snprintf(out + used, size - used, "%s = %s\n", info->key, value);
        if (n < 0 || (size_t)n >= size - used) {
            return 0;
        }
        used += (size_t)n;
    }
    return used;
}

const char* settings_label(SettingId id)
{
    return valid(id) ? k_settings[id].label : "";
}

const char* settings_value_text(Settings s, SettingId id)
{
    if (!valid(id)) {
        return "";
    }
    return k_settings[id].choices[s.value[id]];
}


bool settings_is_switch(SettingId id)
{
    return settings_choice_count(id) == 2 && strcmp(k_settings[id].choices[0], SWITCH_OFF_TEXT) == 0;
}

const char* settings_note(SettingId id)
{
    return valid(id) ? k_settings[id].note : "";
}

static bool valid_page(SettingsPage page)
{
    return (unsigned)page < (unsigned)SETTINGS_PAGE_COUNT;
}

const char* settings_page_name(SettingsPage page)
{
    return valid_page(page) ? k_page_names[page] : "";
}

SettingsPage settings_page_of(SettingId id)
{
    return valid(id) ? k_settings[id].page : SETTINGS_PAGE_COUNT;
}

int settings_page_count(bool developer)
{
    return developer ? SETTINGS_PAGE_COUNT : 1;
}

bool settings_offered(SettingId id, bool developer)
{
    return valid(id) && (int)settings_page_of(id) < settings_page_count(developer);
}

int settings_page_size(SettingsPage page)
{
    int size = 0;
    for (int id = 0; valid_page(page) && id < SETTING_COUNT; id++) {
        size += k_settings[id].page == page;
    }
    return size;
}

SettingId settings_on_page(SettingsPage page, int n)
{
    for (int id = 0; valid_page(page) && n >= 0 && id < SETTING_COUNT; id++) {
        if (k_settings[id].page == page && n-- == 0) {
            return (SettingId)id;
        }
    }
    return SETTING_COUNT;
}

