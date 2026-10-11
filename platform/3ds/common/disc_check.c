#include "disc_check.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static const char* const k_extensions[] = { ".iso", ".gcm", ".ciso" };

static bool ends_with_ci(const char* s, const char* suffix)
{
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    if (n <= m) {
        return false; /* the extension alone is no name */
    }
    for (size_t i = 0; i < m; i++) {
        if (tolower((unsigned char)s[n - m + i]) != suffix[i]) {
            return false;
        }
    }
    return true;
}

bool disc_check_name(const char* name)
{
    if (name == NULL || name[0] == '.' || name[0] == '\0') {
        return false;
    }
    for (size_t i = 0; i < sizeof(k_extensions) / sizeof(k_extensions[0]); i++) {
        if (ends_with_ci(name, k_extensions[i])) {
            return true;
        }
    }
    return false;
}

DiscIdResult disc_check_id(const unsigned char header[8])
{
    bool animal_crossing = header != NULL && (memcmp(header, "GAF", 3) == 0 || memcmp(header, "GAE", 3) == 0);
    if (!animal_crossing || memcmp(header + 4, "01", 2) != 0) {
        return DISC_ID_OTHER_GAME;
    }
    if (header[2] != 'F' || header[3] != 'E') {
        return DISC_ID_OTHER_REGION;
    }
    return header[7] == 0 ? DISC_ID_OK : DISC_ID_OTHER_REVISION;
}

const char* disc_check_region(char region)
{
    switch (region) {
    case 'E': return "USA";
    case 'P': return "European";
    case 'J': return "Japanese";
    case 'U': return "Australian";
    default: return NULL;
    }
}

void disc_check_id_text(const unsigned char header[8], char out[DISC_ID_TEXT])
{
    for (int i = 0; i < DISC_ID_TEXT - 1; i++) {
        out[i] = isalnum(header[i]) ? (char)header[i] : '?';
    }
    out[DISC_ID_TEXT - 1] = '\0';
}

static uint32_t be32(const unsigned char* p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
}

bool disc_check_complete(const unsigned char header[DISC_HEADER_BYTES], uint64_t file_size)
{
    if (header == NULL) {
        return false;
    }
    uint64_t table_end = (uint64_t)be32(header + 0x424) + be32(header + 0x428);
    return file_size >= table_end;
}
