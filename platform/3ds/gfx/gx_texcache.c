#include "gx_texcache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gc_texture.h"
#include "pica_tile.h"
#include "pot_pad.h"
#include "tex_memo.h"

#define MAX_ENTRIES       256 /* the town alone fills 128; the bottom map adds ~40 */
#define HASH_SAMPLE_BYTES 256
#define RGBA8_BYTES       4
#define TLUT_ENTRY_BYTES  2
#define HASH_COMBINE      0x9E3779B1u /* 2^32 / golden ratio: the two samples' hashes don't cancel */

typedef struct {
    const void* data;
    uint16_t width;
    uint16_t height;
    uint8_t format;
    const void* tlut_data;
    uint8_t tlut_format;
    uint16_t tlut_count;
    bool tlut_big_endian;
    uint8_t pad_wrap_s; /* PotPadWrap: how a padded axis is filled (EDGE if unpadded) */
    uint8_t pad_wrap_t;
} TexIdent;

typedef struct {
    TexIdent ident;
    uint32_t data_hash;
    uint32_t tlut_hash;
} TexKey;

typedef struct {
    bool used;
    TexKey key;
    uint32_t key_hash; /* tex_memo_hash of the key: compared before the key itself */
    bool dumped;       /* handed to the dump hook */
    C3D_Tex tex;
    GxtcBinding binding;
    uint32_t last_frame;
    uint32_t bytes;
} Entry;

static Entry s_entries[MAX_ENTRIES];
static uint32_t s_frame;
static uint32_t s_uploads;
static uint64_t s_upload_ticks; /* this frame's decode + tiling time */
static uint32_t s_upload_serial;
static uint32_t s_bytes;
/* The entry each texture found in this drawing pass: within a pass its texels
 * can't change (emu64's conversion buffers only rewind at a pass's start), so
 * a texture drawn again needs no new hash (tex_memo.h). */
static TexMemoSlot s_memo[TEX_MEMO_SLOTS];
static GxtcDumpWantFn s_dump_want;
static GxtcDumpSaveFn s_dump_save;
static uint32_t s_memo_epoch; /* the memo's "frame": one per pass, from 1 */

static uint32_t sample_hash(const void* data, size_t size)
{
    const uint8_t* p = data;
    if (size <= 2 * HASH_SAMPLE_BYTES) {
        return tex_memo_hash(p, size);
    }
    uint32_t first = tex_memo_hash(p, HASH_SAMPLE_BYTES);
    uint32_t last = tex_memo_hash(p + size - HASH_SAMPLE_BYTES, HASH_SAMPLE_BYTES);
    return first ^ (last * HASH_COMBINE);
}

/* Padding zeroed: the identity is hashed and compared whole. 0: can't be decoded. */
static size_t make_ident(const GxbTexObj* obj, const GxbTlut* tlut, TexIdent* id)
{
    size_t size = gc_texture_size((GcTexFormat)obj->format, obj->width, obj->height);
    if (!obj->valid || size == 0) {
        return 0;
    }
    memset(id, 0, sizeof(*id));
    id->data = obj->data;
    id->width = obj->width;
    id->height = obj->height;
    id->format = obj->format;
    id->pad_wrap_s = (uint8_t)pot_pad_effective_wrap(obj->width, (PotPadWrap)obj->wrap_s);
    id->pad_wrap_t = (uint8_t)pot_pad_effective_wrap(obj->height, (PotPadWrap)obj->wrap_t);
    if (obj->is_ci && tlut != NULL && tlut->valid) {
        id->tlut_data = tlut->data;
        id->tlut_format = tlut->format;
        id->tlut_count = tlut->count;
        id->tlut_big_endian = tlut->big_endian;
    }
    return size;
}

/* Like the PC cache: the first and last 256 bytes, and the whole palette. */
static TexKey make_key(const TexIdent* id, size_t size)
{
    TexKey key;
    memset(&key, 0, sizeof(key));
    key.ident = *id;
    key.data_hash = sample_hash(id->data, size);
    if (id->tlut_data != NULL) {
        key.tlut_hash = tex_memo_hash(id->tlut_data, (size_t)id->tlut_count * TLUT_ENTRY_BYTES);
    }
    return key;
}

static void free_entry(Entry* e)
{
    if (e->used) {
        C3D_TexDelete(&e->tex);
        s_bytes -= e->bytes;
        e->used = false;
    }
}

static Entry* find(const TexKey* key, uint32_t key_hash)
{
    for (int i = 0; i < MAX_ENTRIES; i++) {
        const Entry* e = &s_entries[i];
        if (e->used && e->key_hash == key_hash && memcmp(&e->key, key, sizeof(*key)) == 0) {
            return &s_entries[i];
        }
    }
    return NULL;
}

/* An entry used this frame can't have been evicted since (free_slot); the
 * identity check rules out a memo collision. */
static Entry* find_this_frame(uint32_t ident_hash, const TexIdent* id)
{
    int i = tex_memo_get(s_memo, ident_hash, s_memo_epoch);
    if (i == TEX_MEMO_NONE) {
        return NULL;
    }
    Entry* e = &s_entries[i];
    bool same = e->used && e->last_frame == s_frame && memcmp(&e->key.ident, id, sizeof(*id)) == 0;
    return same ? e : NULL;
}

static Entry* free_slot(void)
{
    Entry* oldest = NULL;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        Entry* e = &s_entries[i];
        if (!e->used) {
            return e;
        }
        if (e->last_frame != s_frame && (oldest == NULL || e->last_frame < oldest->last_frame)) {
            oldest = e;
        }
    }
    if (oldest != NULL) {
        free_entry(oldest);
    }
    return oldest; /* NULL if every entry is used this frame */
}

static bool decode_linear(uint32_t* linear, const GxbTexObj* obj, const GxbTlut* tlut)
{
    GcPalette palette = { NULL, GC_TLUT_RGB5A3, 0, true };
    if (tlut != NULL && tlut->valid) {
        palette = (GcPalette){ tlut->data, (GcTlutFormat)tlut->format, tlut->count, tlut->big_endian };
    }
    return gc_texture_decode(linear, obj->data, obj->width, obj->height, (GcTexFormat)obj->format, &palette);
}

/* Each padded axis filled by its wrap mode (pot_pad.h). Caller frees. */
static uint32_t* decode_padded(const GxbTexObj* obj, const GxbTlut* tlut, uint32_t pw, uint32_t ph)
{
    size_t texels = (size_t)obj->width * obj->height;
    uint32_t* linear = malloc(texels * RGBA8_BYTES);
    uint32_t* padded = malloc((size_t)pw * ph * RGBA8_BYTES);
    bool ok = linear != NULL && padded != NULL && decode_linear(linear, obj, tlut) &&
              pot_pad_rgba8_wrap(padded, linear, obj->width, obj->height, (PotPadWrap)obj->wrap_s,
                                 (PotPadWrap)obj->wrap_t);
    free(linear);
    if (!ok) {
        free(padded);
        return NULL;
    }
    return padded;
}

static bool upload(Entry* e, const GxbTexObj* obj, const GxbTlut* tlut)
{
    PotPadAxis s_axis = pot_pad_axis(obj->width, (PotPadWrap)obj->wrap_s);
    PotPadAxis t_axis = pot_pad_axis(obj->height, (PotPadWrap)obj->wrap_t);
    uint32_t pw = s_axis.padded;
    uint32_t ph = t_axis.padded;
    if (pw == 0 || ph == 0) {
        return false;
    }
    uint32_t* padded = decode_padded(obj, tlut, pw, ph);
    if (padded == NULL) {
        return false;
    }
    bool ok = C3D_TexInit(&e->tex, (u16)pw, (u16)ph, GPU_RGBA8);
    if (ok) {
        ok = pica_tile(e->tex.data, padded, pw, ph, RGBA8_BYTES, true);
        if (ok) {
            C3D_TexFlush(&e->tex);
        } else {
            C3D_TexDelete(&e->tex);
        }
    }
    free(padded);
    if (!ok) {
        return false;
    }
    e->bytes = pw * ph * RGBA8_BYTES;
    e->binding = (GxtcBinding){ &e->tex, s_axis.uv_scale, t_axis.uv_scale };
    return true;
}

bool gxtc_init(void)
{
    memset(s_entries, 0, sizeof(s_entries));
    memset(s_memo, 0, sizeof(s_memo));
    s_memo_epoch = 0;
    s_frame = 0;
    s_uploads = 0;
    s_bytes = 0;
    return true;
}

void gxtc_shutdown(void)
{
    for (int i = 0; i < MAX_ENTRIES; i++) {
        free_entry(&s_entries[i]);
    }
}

void gxtc_pass_begin(void)
{
    s_memo_epoch++;
}

void gxtc_set_dump_hooks(GxtcDumpWantFn want, GxtcDumpSaveFn save)
{
    s_dump_want = want;
    s_dump_save = save;
}

/* Once per entry; a failed save is retried on the next upload. */
static void dump(Entry* e, const GxbTexObj* obj, const GxbTlut* tlut)
{
    e->dumped = true;
    GxtcDumpInfo info = { obj->data, e->key.ident.tlut_data, e->key.data_hash ^ (e->key.tlut_hash * HASH_COMBINE),
                          obj->format, obj->width, obj->height };
    if (!s_dump_want(&info)) {
        return;
    }
    uint32_t* linear = malloc((size_t)obj->width * obj->height * RGBA8_BYTES);
    if (linear != NULL && decode_linear(linear, obj, tlut)) {
        s_dump_save(&info, linear);
    }
    free(linear);
}

void gxtc_frame_begin(void)
{
    gxtc_pass_begin();
    s_frame++;
    s_uploads = 0;
    s_upload_ticks = 0;
}

uint32_t gxtc_upload_serial(void)
{
    return s_upload_serial;
}

uint64_t gxtc_upload_ticks_this_frame(void)
{
    return s_upload_ticks;
}

static Entry* lookup(const TexIdent* id, size_t size, const GxbTexObj* obj, const GxbTlut* tlut)
{
    TexKey key = make_key(id, size);
    uint32_t key_hash = tex_memo_hash(&key, sizeof(key));
    Entry* e = find(&key, key_hash);
    if (e != NULL) {
        return e;
    }
    e = free_slot();
    uint64_t start = svcGetSystemTick();
    bool uploaded = e != NULL && upload(e, obj, tlut);
    s_upload_ticks += svcGetSystemTick() - start;
    if (!uploaded) {
        return NULL;
    }
    e->used = true;
    memcpy(&e->key, &key, sizeof(key)); /* padding included: find() uses memcmp */
    e->key_hash = key_hash;
    e->dumped = false;
    s_bytes += e->bytes;
    s_uploads++;
    s_upload_serial++;
    return e;
}

const GxtcBinding* gxtc_get(const GxbTexObj* obj, const GxbTlut* tlut)
{
    TexIdent id;
    size_t size = make_ident(obj, tlut, &id);
    if (size == 0) {
        return NULL;
    }
    uint32_t ident_hash = tex_memo_hash(&id, sizeof(id));
    Entry* e = find_this_frame(ident_hash, &id);
    if (e == NULL) {
        e = lookup(&id, size, obj, tlut);
        if (e == NULL) {
            return NULL;
        }
        tex_memo_put(s_memo, ident_hash, s_memo_epoch, (int)(e - s_entries));
    }
    if (s_dump_want != NULL && s_dump_save != NULL && !e->dumped) {
        dump(e, obj, tlut);
    }
    e->last_frame = s_frame;
    return &e->binding;
}

uint32_t gxtc_uploads_this_frame(void)
{
    return s_uploads;
}

uint32_t gxtc_entry_count(void)
{
    uint32_t n = 0;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        n += s_entries[i].used;
    }
    return n;
}

uint32_t gxtc_bytes(void)
{
    return s_bytes;
}
