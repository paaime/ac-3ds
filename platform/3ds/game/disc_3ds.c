#include "pc_platform.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "pc_disc.h"

#include "disc_3ds.h"
#include "disc_check.h"
#include "status_3ds.h"

#define MESSAGE_MAX 400

static char s_name[320];

struct dirent* ac3ds_disc_readdir(DIR* dir)
{
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && entry->d_name[0] == '.') {
        /* hidden files, ".", "..", and the "._" files macOS writes beside copies */
    }
    return entry;
}

static bool find_image(char* out, size_t size)
{
    static const char* const dirs[] = { ".", "orig", "rom" };
    for (size_t d = 0; d < sizeof(dirs) / sizeof(dirs[0]); d++) {
        DIR* dir = opendir(dirs[d]);
        if (dir == NULL) {
            continue;
        }
        struct dirent* entry;
        while ((entry = ac3ds_disc_readdir(dir)) != NULL) {
            if (disc_check_name(entry->d_name)) {
                int n = snprintf(out, size, "%s%s%.255s", d == 0 ? "" : dirs[d], d == 0 ? "" : "/", entry->d_name);
                closedir(dir);
                return n > 0 && (size_t)n < size;
            }
        }
        closedir(dir);
    }
    return false;
}

static bool check_id(const unsigned char header[8])
{
    char message[MESSAGE_MAX];
    char id[DISC_ID_TEXT];
    disc_check_id_text(header, id);
    switch (disc_check_id(header)) {
    case DISC_ID_OK:
        return true;
    case DISC_ID_OTHER_REGION: {
        const char* region = disc_check_region((char)header[3]);
        snprintf(message, sizeof(message),
                 "%.120s is %s%s version of Animal Crossing (%s).\n\n"
                 "The port only works with the USA version (GAFE01) for now: make it the only disc image "
                 "in the folder.",
                 s_name, region != NULL ? "the " : "a non-USA", region != NULL ? region : "", id);
        break;
    }
    case DISC_ID_OTHER_REVISION:
        snprintf(message, sizeof(message),
                 "%.120s is a later release of the USA version (revision %d).\n\n"
                 "The port only works with the first release (revision 0).",
                 s_name, header[7]);
        break;
    default:
        snprintf(message, sizeof(message),
                 "%.120s isn't Animal Crossing (its game ID is %s).\n\n"
                 "Put your Animal Crossing disc image (USA, GAFE01) in sdmc:/3ds/ac3ds/ instead, as the "
                 "only disc image there.",
                 s_name, id);
        break;
    }
    status3ds_fatal(message);
    return false;
}

/* A truncated copy has pc_disc.c read garbage for minutes; a .ciso's blocks aren't where the header says. */
static bool whole(void)
{
    size_t n = strlen(s_name);
    if (n >= 5 && strcasecmp(s_name + n - 5, ".ciso") == 0) {
        return true;
    }
    unsigned char header[DISC_HEADER_BYTES];
    FILE* f = fopen(s_name, "rb");
    bool read = f != NULL && fread(header, 1, sizeof(header), f) == sizeof(header) && fseek(f, 0, SEEK_END) == 0;
    long size = read ? ftell(f) : -1;
    if (f != NULL) {
        fclose(f);
    }
    return size >= 0 && disc_check_complete(header, (uint64_t)size);
}

bool disc3ds_open(void)
{
    char message[MESSAGE_MAX];
    if (!find_image(s_name, sizeof(s_name))) {
        status3ds_fatal("No disc image found.\n\n"
                        "Put your own Animal Crossing disc image (the USA version, GAFE01: an .iso, .gcm or "
                        ".ciso file) in the folder sdmc:/3ds/ac3ds/ on the SD card. An .rvz or .nkit file "
                        "must be turned into an .iso first (Dolphin can).");
        return false;
    }
    if (!whole()) {
        snprintf(message, sizeof(message),
                 "%.120s is incomplete: it's smaller than the disc it was copied from.\n\n"
                 "Copy it to the SD card again, or dump your disc again.",
                 s_name);
        status3ds_fatal(message);
        return false;
    }
    unsigned char header[8];
    if (!pc_disc_init() || !pc_disc_read(0, header, sizeof(header))) {
        snprintf(message, sizeof(message),
                 "%.120s can't be read as a GameCube disc image.\n\n"
                 "Copy it to the SD card again, as the .iso, .gcm or .ciso file your dump made.",
                 s_name);
        status3ds_fatal(message);
        return false;
    }
    fprintf(stderr, "[boot] disc image %s\n", s_name);
    return check_id(header);
}

const char* disc3ds_name(void)
{
    return s_name;
}
