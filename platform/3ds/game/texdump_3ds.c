/* texdump_3ds.c - the texture dump (see texdump_3ds.h). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "gx_texcache.h"
#include "texdump_3ds.h"
#include "tga.h"

#define DUMP_DIR      "dump" /* in the cwd, the data folder */
#define SEEN_SLOTS    8192u  /* a power of two: textures already saved this session */
#define SEEN_MAX      (SEEN_SLOTS * 3u / 4u)
#define NAME_CHARS    64
#define KEY_MIX       0x9E3779B1u

static struct {
    bool on;
    bool full_logged;
    bool failure_logged;
    uint32_t saved;
    uint32_t seen_count;
    uint64_t seen[SEEN_SLOTS]; /* 0: empty */
} D;

/* Address, palette, size, format and content: a refilled buffer is another texture. */
static uint64_t key_of(const GxtcDumpInfo* t)
{
    uint32_t where = (uint32_t)(uintptr_t)t->data ^ ((uint32_t)(uintptr_t)t->tlut * KEY_MIX) ^
                     ((t->width << 20) + (t->height << 8) + t->format) * KEY_MIX;
    uint64_t key = ((uint64_t)t->content_hash << 32) | where;
    return key != 0 ? key : 1;
}

/* The key's slot: where it is, or the empty one where it would go. */
static uint32_t slot_of(uint64_t key)
{
    uint32_t i = (uint32_t)(key ^ (key >> 29) ^ (key >> 7)) & (SEEN_SLOTS - 1u);
    while (D.seen[i] != 0 && D.seen[i] != key) {
        i = (i + 1u) & (SEEN_SLOTS - 1u);
    }
    return i;
}

static bool want(const GxtcDumpInfo* t)
{
    if (D.seen_count >= SEEN_MAX) {
        if (!D.full_logged) {
            D.full_logged = true;
            fprintf(stderr, "[dump] %lu textures saved: the table is full, the dump stops here\n",
                    (unsigned long)D.saved);
        }
        return false;
    }
    return D.seen[slot_of(key_of(t))] == 0;
}

static bool write_file(const char* name, const uint8_t* file, size_t size)
{
    FILE* f = fopen(name, "wb");
    if (f == NULL) {
        return false;
    }
    bool ok = fwrite(file, 1, size, f) == size;
    ok = fclose(f) == 0 && ok;
    if (!ok) {
        remove(name); /* no half file */
    }
    return ok;
}

static void save(const GxtcDumpInfo* t, const uint32_t* texels)
{
    size_t size = tga_rgba8_size(t->width, t->height);
    uint8_t* file = size != 0 ? malloc(size) : NULL;
    char name[NAME_CHARS];
    snprintf(name, sizeof(name), "%s/%08lX_%08lX_%08lX_%lux%lu_f%u.tga", DUMP_DIR, (unsigned long)(uintptr_t)t->data,
             (unsigned long)(uintptr_t)t->tlut, (unsigned long)t->content_hash, (unsigned long)t->width,
             (unsigned long)t->height, (unsigned)t->format);
    bool ok = file != NULL && tga_write_rgba8(file, size, texels, t->width, t->height) == size &&
              write_file(name, file, size);
    free(file);
    if (ok) {
        uint64_t key = key_of(t);
        D.seen[slot_of(key)] = key; /* only once it's on the card: a failed one is tried again later */
        D.seen_count++;
        D.saved++;
    } else if (!D.failure_logged) {
        D.failure_logged = true;
        fprintf(stderr, "[dump] couldn't save %s (SD card full?)\n", name);
    }
}

void texdump3ds_enable(bool on)
{
    if (on == D.on) {
        return;
    }
    D.on = on;
    if (on) {
        mkdir(DUMP_DIR, 0777); /* already there is fine */
        fprintf(stderr, "[dump] saving each texture used to %s/\n", DUMP_DIR);
    } else {
        fprintf(stderr, "[dump] stopped: %lu textures saved\n", (unsigned long)D.saved);
    }
    gxtc_set_dump_hooks(on ? want : NULL, on ? save : NULL);
}
