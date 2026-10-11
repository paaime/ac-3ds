#include "gx_dlist.h"

#include <string.h>

#define MAGIC       0x41433344u /* "AC3D" */
#define WORD        sizeof(uint32_t)
#define OP_SHIFT    16
#define COUNT_MASK  0xFFFFu

static void put_word(GxDlRecorder* r, uint32_t w)
{
    memcpy(r->buf + r->used, &w, WORD);
    r->used += WORD;
}

static uint32_t get_word(const uint8_t* p)
{
    uint32_t w;
    memcpy(&w, p, WORD);
    return w;
}

GxDlRecorder gx_dl_begin(void* buf, uint32_t size)
{
    GxDlRecorder r = { (uint8_t*)buf, size, 0, buf != NULL, false };
    if (r.active && size >= WORD) {
        put_word(&r, MAGIC);
    } else {
        r.overflow = true;
    }
    return r;
}

bool gx_dl_recording(const GxDlRecorder* r)
{
    return r->active;
}

bool gx_dl_record(GxDlRecorder* r, uint32_t op, const uint32_t* args, uint32_t count)
{
    if (!r->active || r->overflow) {
        return false;
    }
    uint32_t need = WORD * (1 + count);
    if (count > GX_DL_MAX_ARGS || r->size - r->used < need) {
        r->overflow = true;
        return false;
    }
    put_word(r, (op << OP_SHIFT) | count);
    for (uint32_t i = 0; i < count; i++) {
        put_word(r, args[i]);
    }
    return true;
}

uint32_t gx_dl_end(GxDlRecorder* r)
{
    uint32_t bytes = r->active && !r->overflow ? r->used : 0;
    r->active = false;
    return bytes;
}

uint32_t gx_dl_replay(const void* list, uint32_t nbytes, GxDlHandler handler, void* ctx)
{
    const uint8_t* p = list;
    if (p == NULL || nbytes < WORD || get_word(p) != MAGIC) {
        return 0;
    }
    uint32_t off = WORD;
    uint32_t replayed = 0;
    while (nbytes - off >= WORD) {
        uint32_t header = get_word(p + off);
        uint32_t count = header & COUNT_MASK;
        if (count > GX_DL_MAX_ARGS || nbytes - off - WORD < WORD * count) {
            break; /* malformed or cut short */
        }
        uint32_t args[GX_DL_MAX_ARGS];
        memcpy(args, p + off + WORD, WORD * count);
        handler(header >> OP_SHIFT, args, count, ctx);
        off += WORD * (1 + count);
        replayed++;
    }
    return replayed;
}
